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
            first_collect(state, network);
            break;
        case fwd_mode::FIRST_FORWARD:
            first_forward(state);
            break;
        case fwd_mode::COLLECT_NHOPS:
            collect_next_hops(state, network);
            break;
        case fwd_mode::FORWARD_PACKET:
            forward_packet(state);
            break;
        case fwd_mode::ACCEPTED:
            accepted(state, network);
            break;
        case fwd_mode::DROPPED:
            set_executable(state, 0);
            break;
        default:
            Logger::error("Unknown forwarding mode: " + std::to_string(mode));
    }
}

void ForwardingProcess::first_collect(State *state, const Network& network)
{
    collect_next_hops(state, network);
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

void ForwardingProcess::collect_next_hops(State *state, const Network& network)
{
    Node *current_node = get_pkt_location(state);
    if (typeid(*current_node) == typeid(Middlebox)) {
        // current_node is a Middlebox; inject packet to update next hops
        inject_packet(state, static_cast<Middlebox *>(current_node), network);
        return;
    } // else: current_node is a Node; look up next hops from FIB

    std::set<FIB_IPNH> next_hops = get_fib(state)->lookup(current_node);
    if (next_hops.empty()) {
        Logger::info("Connection " + std::to_string(get_conn(state)) +
                     " dropped by " + current_node->to_string());
        set_fwd_mode(state, fwd_mode::DROPPED);
        set_executable(state, 0);
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
        if (typeid(*next_hop) == typeid(Middlebox)) {
            set_executable(state, 1);
        }

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
    State *state, const Network& network, uint8_t next_proto_state,
    bool change_direction) const
{
    // check if the new src_node is a middlebox
    // if it is, do not inject packet and mark the connection as "not
    // executable" (missing packet)
    Node *new_src_node = change_direction ? get_pkt_location(state) : get_src_node(state);
    if (typeid(*new_src_node) == typeid(Middlebox)) {
        Logger::debug("Freeze conn " + std::to_string(get_conn(state)));
        set_executable(state, 0);
        return;
    }

    int old_proto_state = get_proto_state(state);

    // compute seq and ack numbers
    if (PS_IS_TCP(old_proto_state)) {
        uint32_t seq = get_seq(state);
        uint32_t ack = get_ack(state);
        Payload *pl = PayloadMgr::get().get_payload(state);
        uint32_t payload_size = pl ? pl->get_size() : 0;
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

void ForwardingProcess::inject_packet(
    State *state, Middlebox *mb, const Network& network)
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
    Logger::info("Injecting packet: " + new_pkt->to_string());
    std::vector<Packet> recv_pkts = mb->send_pkt(*new_pkt);
    process_recv_pkts(state, mb, std::move(recv_pkts), network);

    //// logging
    //std::string nhops_str;
    //nhops_str += mb->to_string() + " -> [";
    //for (const FIB_IPNH& nhop : next_hops) {
    //    nhops_str += " " + nhop.to_string();
    //}
    //nhops_str += " ]";
    //Logger::info(nhops_str);

    Stats::set_overall_latency();
}

/*
 * No multicast.
 */
void ForwardingProcess::process_recv_pkts(
    State *state, Middlebox *mb, std::vector<Packet>&& recv_pkts,
    const Network& network) const
{
    for (Packet& recv_pkt : recv_pkts) {
        if (recv_pkt.empty()) {
            Logger::info("Received packet: (empty)");
            continue;
        }

        // identify connection and sanitize packet
        int conn;
        bool is_new, opposite_dir, next_phase;
        identify_conn(state, recv_pkt, conn, is_new, opposite_dir);
        uint8_t old_proto_state = state->conn_state[conn].proto_state;
        Net::get().convert_proto_state(recv_pkt, is_new, opposite_dir, old_proto_state);
        check_proto_state(recv_pkt, is_new, old_proto_state, next_phase);
        check_seq_ack(state, recv_pkt, conn, is_new, opposite_dir, next_phase);
        Logger::info("Received packet [conn " + std::to_string(conn) + "]: "
                     + recv_pkt.to_string());

        // map the packet info (5-tuple + seq/ack + FIB + control logic) back
        // to the system state
        recv_pkt.update_conn(state, conn, network);

        // set conn (and recover it later)
        int orig_conn = get_conn(state);
        set_conn(state, conn);

        // update the remaining connection state variables based on the inferred
        // connection info
        if (is_new) {
            set_num_conns(state, get_num_conns(state) + 1);
            set_src_node(state, mb);
            set_tx_node(state, mb);
            set_rx_node(state, nullptr);
            set_pkt_location(state, mb);
            set_fwd_mode(state, fwd_mode::FIRST_FORWARD);
            set_ingress_intf(state, nullptr);
            set_path_choices(state, Choices());
        } else if (next_phase) {
            if (opposite_dir) {
                set_src_node(state, mb);
            } else {
                set_pkt_location(state, get_src_node(state));
            }
            set_fwd_mode(state, fwd_mode::FIRST_FORWARD);
            set_ingress_intf(state, nullptr);
        } else {    // same phase (middlebox is not an endpoint)
            assert(get_fwd_mode(state) == fwd_mode::COLLECT_NHOPS);
            set_fwd_mode(state, fwd_mode::FORWARD_PACKET);
        }

        // find the next hop and set candidates
        Candidates candidates;
        FIB_IPNH next_hop = mb->get_ipnh(recv_pkt.get_intf()->get_name(),
                                         recv_pkt.get_dst_ip());
        if (next_hop.get_l3_node()) {
            candidates.add(next_hop);
        } else {
            Logger::warn("No next hop found");
        }
        set_candidates(state, std::move(candidates));

        set_conn(state, orig_conn);
    }

    print_conn_states(state);

    // control logic:
    // if the current connection isn't updated, assume the packet is accepted
    if (get_fwd_mode(state) == fwd_mode::COLLECT_NHOPS) {
        set_executable(state, 2);
        set_fwd_mode(state, fwd_mode::FORWARD_PACKET);
        Candidates candidates;
        candidates.add(FIB_IPNH(mb, nullptr, mb, nullptr));
        set_candidates(state, std::move(candidates));
    }

    // update choice_count for the current connection
    set_choice_count(state, get_candidates(state)->size());
}

/*
 * It makes sure the 5-tuple values of the received packet comply with the
 * system.
 */
void ForwardingProcess::identify_conn(
    State *state, const Packet& pkt, int& conn, bool& is_new, bool& opposite_dir) const
{
    int orig_conn = get_conn(state);
    is_new = false;

    for (conn = 0; conn < get_num_conns(state); ++conn) {
        set_conn(state, conn);
        uint32_t src_ip = get_src_ip(state);
        EqClass *dst_ip_ec = get_dst_ip_ec(state);
        uint16_t src_port = get_src_port(state);
        uint16_t dst_port = get_dst_port(state);
        int conn_protocol = PS_TO_PROTO(get_proto_state(state));
        set_conn(state, orig_conn);

        /*
         * Note that at this moment we have not converted the received packet's
         * proto_state, so only protocol type is compared.
         */
        int pkt_protocol;
        if (pkt.get_proto_state() & 0x80U) {
            pkt_protocol = proto::tcp;
        } else {
            pkt_protocol = PS_TO_PROTO(pkt.get_proto_state());
        }

        if (pkt.get_src_ip() == src_ip
                && dst_ip_ec->contains(pkt.get_dst_ip())
                && pkt.get_src_port() == src_port
                && pkt.get_dst_port() == dst_port
                && pkt_protocol == conn_protocol) {
            // same connection, same direction
            opposite_dir = false;
            return;
        } else if (pkt.get_dst_ip() == src_ip
                   && dst_ip_ec->contains(pkt.get_src_ip())
                   && pkt.get_dst_port() == src_port
                   && pkt.get_src_port() == dst_port
                   && pkt_protocol == conn_protocol) {
            // same connection, opposite direction
            opposite_dir = true;
            return;
        }
    }

    // new connection (NAT'd packets are also treated as new connections)
    if (conn >= MAX_CONNS) {
        Logger::error("Exceeding the maximum number of connections");
    }
    is_new = true;
    opposite_dir = false;
}

/*
 * It makes sure the protocol state of the received packet comply with the
 * system.
 */
void ForwardingProcess::check_proto_state(
    const Packet& pkt, bool is_new, uint8_t old_proto_state, bool& next_phase) const
{
    if (is_new) {
        assert(PS_IS_FIRST(pkt.get_proto_state()));
        next_phase = false;
    } else {
        if (pkt.get_proto_state() == old_proto_state + 1) {
            next_phase = true;
        } else if (pkt.get_proto_state() == old_proto_state) {
            next_phase = false;
        } else {
            Logger::error("Invalid protocol state");
        }
    }
}

/*
 * It makes sure the seq/ack numbers of the received packet comply with the
 * system.
 */
void ForwardingProcess::check_seq_ack(
    State *state, const Packet& pkt, int conn, bool is_new, bool opposite_dir,
    bool next_phase) const
{
    // verify seq/ack numbers if it's a TCP packet
    if (PS_IS_TCP(pkt.get_proto_state())) {
        int orig_conn = get_conn(state);
        set_conn(state, conn);

        if (is_new) {               // new connection (SYN)
            assert(pkt.get_ack() == 0);
        } else if (next_phase) {    // old connection; next phase
            uint8_t old_proto_state = get_proto_state(state);
            uint32_t old_seq = get_seq(state);
            uint32_t old_ack = get_ack(state);
            Payload *pl = PayloadMgr::get().get_payload(state);
            uint32_t old_payload_size = pl ? pl->get_size() : 0;
            if (old_payload_size > 0) {
                old_seq += old_payload_size;
            } else if (PS_HAS_SYN(old_proto_state) || PS_HAS_FIN(old_proto_state)) {
                old_seq += 1;
            }
            if (opposite_dir) {
                if (pkt.get_proto_state() != PS_TCP_INIT_2) {
                    assert(pkt.get_seq() == old_ack);
                }
                assert(pkt.get_ack() == old_seq);
            } else {
                assert(pkt.get_seq() == old_seq);
                assert(pkt.get_ack() == old_ack);
            }
        } else {                    // old connection; same phase
            // seq/ack should remain the same
            assert(pkt.get_seq() == get_seq(state));
            assert(pkt.get_ack() == get_ack(state));
        }

        set_conn(state, orig_conn);
    }
}
