#include "process/forwarding.hpp"

#include <cassert>
#include <typeinfo>
#include <utility>
#include <optional>

#include "candidates.hpp"
#include "choices.hpp"
#include "eqclassmgr.hpp"
#include "fib.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "payload.hpp"
#include "protocols.hpp"
#include "stats.hpp"
#include "lib/net.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

ForwardingProcess::~ForwardingProcess()
{
    for (auto& packet : all_pkts) {
        delete packet;
    }
    for (auto& node_pkt_hist : node_pkt_hist_hist) {
        delete node_pkt_hist;
    }
}

void ForwardingProcess::init(State *state, const Network& network)
{
    if (!enabled) {
        return;
    }

    PacketHistory pkt_hist(network);
    set_pkt_hist(state, std::move(pkt_hist));
}

void ForwardingProcess::exec_step(State *state, const Network& network)
{
    if (!enabled) {
        return;
    }

    int mode = get_fwd_mode(state);
    switch (mode) {
        case fwd_mode::FIRST_COLLECT:
            first_collect(state);
            break;
        case fwd_mode::FIRST_FORWARD:
            first_forward(state);
            break;
        case fwd_mode::COLLECT_NHOPS:
            collect_next_hops(state);
            break;
        case fwd_mode::FORWARD_PACKET:
            forward_packet(state);
            break;
        case fwd_mode::ACCEPTED:
            accepted(state, network);
            break;
        case fwd_mode::DROPPED:
            state->choice_count = 0;
            break;
        default:
            Logger::error("Unknown forwarding mode: " + std::to_string(mode));
    }
}

void ForwardingProcess::first_collect(State *state)
{
    collect_next_hops(state);
    set_fwd_mode(state, fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward(State *state)
{
    if (PS_IS_FIRST(get_proto_state(state))) {
        // update the source IP address according to the egress interface
        Candidates *candidates = get_candidates(state);
        FIB_IPNH next_hop = candidates->at(state->choice);
        if (next_hop.get_l2_intf()) {
            const Interface *egress_intf
                = next_hop.get_l2_node()->get_peer(
                      next_hop.get_l2_intf()->get_name()
                  ).second;
            set_src_ip(state, egress_intf->addr().get_value());
        } // else: sender == receiver
    }

    forward_packet(state);
}

void ForwardingProcess::collect_next_hops(State *state)
{
    std::set<FIB_IPNH> next_hops;
    Node *current_node = get_pkt_location(state);

    if (typeid(*current_node) == typeid(Node)) {
        // current_node is a Node; look up next hops from the fib
        FIB *fib = get_fib(state);
        next_hops = fib->lookup(current_node);
    } else {
        // current_node is a Middlebox; inject packet to get next hops
        next_hops = inject_packet(state, static_cast<Middlebox *>(current_node));
    }

    if (next_hops.empty()) {
        Logger::info("Packet dropped by " + current_node->to_string());
        set_fwd_mode(state, fwd_mode::DROPPED);
        state->choice_count = 0;
        return;
    }

    Candidates candidates;
    EqClass *ec = get_dst_ip_ec(state);
    Choices *choices = get_path_choices(state);
    std::optional<FIB_IPNH> choice = choices->get_choice(ec, current_node);

    // in case of multipath, use the past choice if it's been made
    if (next_hops.size() > 1 && choice && next_hops.count(*choice) > 0) {
        candidates.add(*choice);
    } else {
        for (const FIB_IPNH& next_hop : next_hops) {
            candidates.add(next_hop);
        }
    }

    set_candidates(state, std::move(candidates));
    set_fwd_mode(state, fwd_mode::FORWARD_PACKET);
}

void ForwardingProcess::forward_packet(State *state)
{
    Node *current_node = get_pkt_location(state);
    Candidates *candidates = get_candidates(state);

    // in case of multipath, remember the path choice
    if (candidates->size() > 1) {
        Choices new_choices(*get_path_choices(state));
        new_choices.add_choice(get_dst_ip_ec(state), current_node, candidates->at(state->choice));
        set_path_choices(state, std::move(new_choices));
    }

    Node *next_hop = candidates->at(state->choice).get_l3_node();
    Interface *ingress_intf = candidates->at(state->choice).get_l3_intf();

    if (next_hop == current_node) {
        // check if the endpoints remain consistent
        int proto_state = get_proto_state(state);
        Node *current_node = get_pkt_location(state);
        Node *tx_node = get_tx_node(state);
        Node *rx_node = get_rx_node(state);
        if (PS_IS_FIRST(proto_state)) {
            // store the original receiving endpoint of the connection
            set_rx_node(state, current_node);
        } else if (PS_IS_REQUEST_DIR(proto_state) && current_node != rx_node) {
            Logger::error("Inconsistent endpoints (current_node != rx_node)");
        } else if (PS_IS_REPLY_DIR(proto_state) && current_node != tx_node) {
            Logger::error("Inconsistent endpoints (current_node != tx_node)");
        }

        Logger::info("Packet delivered at " + current_node->to_string());
        set_fwd_mode(state, fwd_mode::ACCEPTED);
        state->choice_count = 1;
    } else {
        set_pkt_location(state, next_hop);
        set_ingress_intf(state, ingress_intf);
        Logger::info("Packet forwarded to " + next_hop->to_string() + ", "
                     + ingress_intf->to_string());
        set_fwd_mode(state, fwd_mode::COLLECT_NHOPS);
        state->choice_count = 1;
    }
}

void ForwardingProcess::accepted(State *state, const Network& network)
{
    int proto_state = get_proto_state(state);
    switch (proto_state) {
        case PS_TCP_INIT_1:
            phase_transition(state, network, PS_TCP_INIT_2, true);
            break;
        case PS_TCP_INIT_2:
            phase_transition(state, network, PS_TCP_INIT_3, true);
            break;
        case PS_TCP_INIT_3:
            phase_transition(state, network, PS_TCP_L7_REQ, false);
            break;
        case PS_TCP_L7_REQ:
            phase_transition(state, network, PS_TCP_L7_REQ_A, true);
            break;
        case PS_TCP_L7_REQ_A:
            phase_transition(state, network, PS_TCP_L7_REP, false);
            break;
        case PS_TCP_L7_REP:
            phase_transition(state, network, PS_TCP_L7_REP_A, true);
            break;
        case PS_TCP_L7_REP_A:
            phase_transition(state, network, PS_TCP_TERM_1, true);
            break;
        case PS_TCP_TERM_1:
            phase_transition(state, network, PS_TCP_TERM_2, true);
            break;
        case PS_TCP_TERM_2:
            phase_transition(state, network, PS_TCP_TERM_3, true);
            break;
        case PS_TCP_TERM_3:
            state->choice_count = 0;
            break;
        case PS_UDP_REQ:
            phase_transition(state, network, PS_UDP_REP, true);
            break;
        case PS_UDP_REP:
            state->choice_count = 0;
            break;
        case PS_ICMP_ECHO_REQ:
            phase_transition(state, network, PS_ICMP_ECHO_REP, true);
            break;
        case PS_ICMP_ECHO_REP:
            state->choice_count = 0;
            break;
        default:
            Logger::error("Unknown protocol state " + std::to_string(proto_state));
    }
}

void ForwardingProcess::phase_transition(
    State *state, const Network& network, uint8_t next_proto_state, bool change_direction)
{
    int old_proto_state = get_proto_state(state);

    // compute seq and ack numbers
    if (PS_IS_TCP(old_proto_state)) {
        uint32_t seq = get_seq(state);
        uint32_t ack = get_ack(state);
        uint32_t payload_size = PayloadMgr::get().get_payload(state)->get_size();
        if (payload_size > 0) {
            seq += payload_size;
        } else if (PS_HAS_SYN(old_proto_state) || PS_HAS_FIN(old_proto_state)) {
            seq += 1;
        }
        if (change_direction) {
            std::swap(seq, ack);
        }
        set_seq(state, seq);
        set_ack(state, ack);
    }

    set_proto_state(state, next_proto_state);

    // update src/dst IP, src node, src/dst ports, FIB, and pkt loc
    if (change_direction) {
        uint32_t src_ip = get_src_ip(state);
        EqClass *dst_ip_ec = get_dst_ip_ec(state);
        // the next src IP
        set_src_ip(state, dst_ip_ec->representative_addr().get_value());
        // the next dst IP EC
        EqClass *next_dst_ip_ec = EqClassMgr::get().find_ec(src_ip);
        set_dst_ip_ec(state, next_dst_ip_ec);
        Logger::info("EC: " + next_dst_ip_ec->to_string());
        // src node
        set_src_node(state, get_pkt_location(state));
        // src port and dst port
        uint16_t src_port = get_src_port(state);
        uint16_t dst_port = get_dst_port(state);
        set_src_port(state, dst_port);
        set_dst_port(state, src_port);
        // FIB
        network.update_fib(state);
    } else {
        set_pkt_location(state, get_src_node(state));
    }

    set_fwd_mode(state, fwd_mode::FIRST_COLLECT);
    set_ingress_intf(state, nullptr);
    reset_candidates(state);
}

void ForwardingProcess::identify_conn(
    State *state, const Packet& pkt, int& conn, bool& change_direction) const
{
    for (conn = 0; conn < state->num_conns; ++conn) {
        EqClass *ec = get_dst_ip_ec(state);
        uint32_t src_ip = get_src_ip(state);
        uint16_t src_port = get_src_port(state);
        uint16_t dst_port = get_dst_port(state);

        if (ec->contains(pkt.get_dst_ip()) && pkt.get_src_ip() == src_ip
                && pkt.get_src_port() == src_port
                && pkt.get_dst_port() == dst_port) {
            // same connection, same direction
            change_direction = false;
            return;
        } else if (ec->contains(pkt.get_src_ip()) && pkt.get_dst_ip() == src_ip
                   && pkt.get_dst_port() == src_port
                   && pkt.get_src_port() == dst_port) {
            // same connection, different direction
            change_direction = true;
            return;
        }
        // TODO: match NAT'd packet to the original connection, by matching
        // the seq/ack numbers?
    }

    conn = -1;  // new connection or NAT
    Logger::error("New connection or NAT (not implemented)");
    //// NAT (network address translation)
    //// NOTE: we only handle the destination IP rewriting for now.
    //EqClass *ec;
    //memcpy(&ec, state->conn_state[state->conn].ec, sizeof(EqClass *));
    //if (!ec->contains(recv_pkt.get_dst_ip())) {
    //    // set the next EC
    //    policy->add_ec(state, recv_pkt.get_dst_ip());
    //    EqClass *next_ec = policy->find_ec(state, recv_pkt.get_dst_ip());
    //    memcpy(state->conn_state[state->conn].ec, &next_ec, sizeof(EqClass *));
    //    Logger::info("NAT EC: " + next_ec->to_string());
    //    // update FIB according to the new EC
    //    network.update_fib(state);
    //}
}

std::set<FIB_IPNH> ForwardingProcess::inject_packet(State *state, Middlebox *mb)
{
    Stats::set_overall_lat_t1();

    // check out current pkt_hist and rewind the middlebox state
    PacketHistory *pkt_hist = get_pkt_hist(state);
    NodePacketHistory *current_nph = pkt_hist->get_node_pkt_hist(mb);
    Stats::set_rewind_lat_t1();
    int rewind_injections = mb->rewind(current_nph);
    Stats::set_rewind_latency();
    Stats::set_rewind_injection_count(rewind_injections);

    // construct new packet
    Packet *new_pkt = new Packet(state);
    auto res = all_pkts.insert(new_pkt);
    if (!res.second) {
        delete new_pkt;
        new_pkt = *(res.first);
    }

    // update node_pkt_hist with this new packet
    NodePacketHistory *new_nph = new NodePacketHistory(new_pkt, current_nph);
    auto res2 = node_pkt_hist_hist.insert(new_nph);
    if (!res2.second) {
        delete new_nph;
        new_nph = *(res2.first);
    }
    mb->set_node_pkt_hist(new_nph);

    // update pkt_hist with this new node_pkt_hist
    PacketHistory new_pkt_hist(*pkt_hist);
    new_pkt_hist.set_node_pkt_hist(mb, new_nph);
    set_pkt_hist(state, std::move(new_pkt_hist));

    // inject packet
    std::vector<Packet> recv_pkts = mb->send_pkt(*new_pkt);

    std::set<FIB_IPNH> next_hops;

    // process received packets
    for (Packet& recv_pkt : recv_pkts) {
        if (recv_pkt.empty()) {
            Logger::info("Received packet: (empty)");
            continue;
        }

        // identify connection
        int conn;
        bool change_direction;
        identify_conn(state, recv_pkt, conn, change_direction);
        assert(conn == state->conn); // same connection

        // convert TCP flags to the real proto_state
        Net::get().convert_proto_state(recv_pkt, get_proto_state(state), change_direction);
        Logger::info("Received packet: " + recv_pkt.to_string());

        // find the next hop
        FIB_IPNH next_hop = mb->get_ipnh(recv_pkt.get_intf()->get_name(), recv_pkt.get_dst_ip());
        if (next_hop.get_l3_node()) {
            next_hops.insert(next_hop);
        } else {
            Logger::warn("No next hop found");
        }

        // next phase (if the middlebox is not "in the middle")
        if (PS_IS_NEXT(recv_pkt.get_proto_state(), get_proto_state(state))) {
            Logger::error("Next phase (not implemented yet)");
            // TODO: map back the packet content to the spin vector
            // src_ip, dst_ip, src_port, dst_port, seq, ack, proto_state
//            std::vector<FIB_IPNH> candidates(1, FIB_IPNH(mb, nullptr, mb, nullptr));
//            set_candidates(state, candidates);
//            if (get_fwd_mode(state) == fwd_mode::FIRST_COLLECT) {
//                set_fwd_mode(state, fwd_mode::FIRST_FORWARD);
//            } else {
//                set_fwd_mode(state, fwd_mode::FORWARD_PACKET);
//            }
//            exec_step(state, network);
//            exec_step(state, network);  /* should be accepted by now */
//
//            // update seq and ack according to the received packet
//            if (PS_HAS_SYN(recv_pkt.get_proto_state())) {
//                uint32_t seq = recv_pkt.get_seq(), ack = recv_pkt.get_ack();
//                set_seq(state, seq);
//                set_ack(state, ack);
//            }
//
//            // update src_node and ingress_intf (as in packet_entry)
//            memcpy(state->conn_state[state->conn].src_node, mb, sizeof(Node *));
//            Interface *ingress_intf = recv_pkt.get_intf();
//            memcpy(state->conn_state[state->conn].ingress_intf, &ingress_intf, sizeof(Interface *));
        }
    }

    // if there is no response when the injected packet is destined to the
    // middlebox, assume the sent packet is accepted if it's an ACK
    if (next_hops.empty() && mb->has_ip(new_pkt->get_dst_ip())
            && new_pkt->get_proto_state() == PS_TCP_INIT_3) {
        next_hops.insert(FIB_IPNH(mb, nullptr, mb, nullptr));
    }

    // logging
    std::string nhops_str;
    nhops_str += mb->to_string() + " -> [";
    for (const FIB_IPNH& nhop : next_hops) {
        nhops_str += " " + nhop.to_string();
    }
    nhops_str += " ]";
    Logger::info(nhops_str);

    Stats::set_overall_latency();
    return next_hops;
}
