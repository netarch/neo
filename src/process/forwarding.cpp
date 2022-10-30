#include "process/forwarding.hpp"

#include <cassert>
#include <optional>
#include <typeinfo>
#include <utility>

#include "candidates.hpp"
#include "choices.hpp"
#include "eqclassmgr.hpp"
#include "fib.hpp"
#include "lib/logger.hpp"
#include "lib/net.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"
#include "network.hpp"
#include "payload.hpp"
#include "protocols.hpp"
#include "stats.hpp"

ForwardingProcess::~ForwardingProcess() {
    for (auto &packet : all_pkts) {
        delete packet;
    }
    for (auto &node_pkt_hist : node_pkt_hist_hist) {
        delete node_pkt_hist;
    }
}

void ForwardingProcess::init(const Network &network) {
    if (!enabled) {
        return;
    }

    PacketHistory pkt_hist(network);
    model.set_pkt_hist(std::move(pkt_hist));
}

void ForwardingProcess::exec_step() {
    if (!enabled) {
        return;
    }

    int mode = model.get_fwd_mode();
    switch (mode) {
    case fwd_mode::FIRST_COLLECT:
        first_collect();
        break;
    case fwd_mode::FIRST_FORWARD:
        first_forward();
        break;
    case fwd_mode::COLLECT_NHOPS:
        collect_next_hops();
        break;
    case fwd_mode::FORWARD_PACKET:
        forward_packet();
        break;
    case fwd_mode::ACCEPTED:
        accepted();
        break;
    case fwd_mode::DROPPED:
        model.set_executable(0);
        break;
    default:
        Logger::error("Unknown forwarding mode: " + std::to_string(mode));
    }
}

void ForwardingProcess::first_collect() {
    collect_next_hops();
    model.set_fwd_mode(fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward() {
    if (PS_IS_FIRST(model.get_proto_state()) && model.get_src_ip() == 0) {
        // update the source IP address according to the egress interface
        Candidates *candidates = model.get_candidates();
        FIB_IPNH next_hop = candidates->at(model.get_choice());
        if (next_hop.get_l2_intf()) {
            const Interface *egress_intf =
                next_hop.get_l2_node()
                    ->get_peer(next_hop.get_l2_intf()->get_name())
                    .second;
            model.set_src_ip(egress_intf->addr().get_value());
        } // else: sender == receiver
    }

    forward_packet();
}

void ForwardingProcess::collect_next_hops() {
    Node *current_node = model.get_pkt_location();
    if (typeid(*current_node) == typeid(Middlebox)) {
        // current_node is a Middlebox; inject packet to update next hops
        inject_packet(static_cast<Middlebox *>(current_node));
        return;
    } // else: current_node is a Node; look up next hops from FIB

    std::set<FIB_IPNH> next_hops = model.get_fib()->lookup(current_node);
    if (next_hops.empty()) {
        Logger::info("Connection " + std::to_string(model.get_conn()) +
                     " dropped by " + current_node->to_string());
        model.set_fwd_mode(fwd_mode::DROPPED);
        model.set_executable(0);
        return;
    }

    Candidates candidates;
    EqClass *ec = model.get_dst_ip_ec();
    Choices *choices = model.get_path_choices();
    std::optional<FIB_IPNH> choice = choices->get_choice(ec, current_node);

    // in case of multipath, use the past choice if it's been made
    if (next_hops.size() > 1 && choice && next_hops.count(*choice) > 0) {
        candidates.add(*choice);
    } else {
        for (const FIB_IPNH &next_hop : next_hops) {
            candidates.add(next_hop);
        }
    }

    model.set_candidates(std::move(candidates));
    model.set_fwd_mode(fwd_mode::FORWARD_PACKET);
}

void ForwardingProcess::forward_packet() {
    Node *current_node = model.get_pkt_location();
    Candidates *candidates = model.get_candidates();

    // in case of multipath, remember the path choice
    if (candidates->size() > 1) {
        Choices new_choices(*model.get_path_choices());
        new_choices.add_choice(model.get_dst_ip_ec(), current_node,
                               candidates->at(model.get_choice()));
        model.set_path_choices(std::move(new_choices));
    }

    Node *next_hop = candidates->at(model.get_choice()).get_l3_node();
    Interface *ingress_intf = candidates->at(model.get_choice()).get_l3_intf();

    // When the packet is delivered at its destination
    if (next_hop == current_node) {
        // check if the endpoints remain consistent
        int proto_state = model.get_proto_state();
        Node *current_node = model.get_pkt_location();
        Node *tx_node = model.get_tx_node();
        Node *rx_node = model.get_rx_node();
        if (PS_IS_FIRST(proto_state)) {
            // store the original receiving endpoint of the connection
            model.set_rx_node(current_node);
        } else if ((PS_IS_REQUEST_DIR(proto_state) &&
                    current_node != rx_node) ||
                   (PS_IS_REPLY_DIR(proto_state) && current_node != tx_node)) {
            // Note: Either tx or rx node can initiate the termination process,
            // so don't check endpoint consistency for PS_TCP_TERM_*
            // inconsistent endpoints: dropped by middlebox
            Logger::info("Inconsistent endpoints");
            Logger::info("Connection " + std::to_string(model.get_conn()) +
                         " dropped by " + current_node->to_string());
            model.set_fwd_mode(fwd_mode::DROPPED);
            model.set_executable(0);
            return;
        }

        Logger::info("Packet delivered at " + current_node->to_string());
        model.set_fwd_mode(fwd_mode::ACCEPTED);
        model.set_choice_count(1);
    } else {
        if (typeid(*next_hop) == typeid(Middlebox)) {
            // packet delivered at middlebox, set rx node
            if (PS_IS_FIRST(model.get_proto_state())) {
                // store the original receiving endpoint of the connection
                model.set_rx_node(next_hop);
            }
            model.set_executable(1);
        }

        model.set_pkt_location(next_hop);
        model.set_ingress_intf(ingress_intf);
        Logger::info("Packet forwarded to " + next_hop->to_string() + ", " +
                     ingress_intf->to_string());
        model.set_fwd_mode(fwd_mode::COLLECT_NHOPS);
        model.set_choice_count(1);
    }
}

void ForwardingProcess::accepted() {
    int proto_state = model.get_proto_state();
    Node &pkt_loc = *model.get_pkt_location();

    switch (proto_state) {
    case PS_TCP_INIT_1:
        phase_transition(PS_TCP_INIT_2, true);
        break;
    case PS_TCP_INIT_2:
        phase_transition(PS_TCP_INIT_3, true);
        break;
    case PS_TCP_INIT_3:
        phase_transition(PS_TCP_L7_REQ, false);
        break;
    case PS_TCP_L7_REQ:
        phase_transition(PS_TCP_L7_REQ_A, true);
        break;
    case PS_TCP_L7_REQ_A:
        phase_transition(PS_TCP_L7_REP, false);
        break;
    case PS_TCP_L7_REP:
        phase_transition(PS_TCP_L7_REP_A, true);
        break;
    case PS_TCP_L7_REP_A:
        if (typeid(pkt_loc) == typeid(Middlebox)) {
            phase_transition(PS_TCP_TERM_1, false);
        } else {
            phase_transition(PS_TCP_TERM_1, true);
        }
        break;
    case PS_TCP_TERM_1:
        phase_transition(PS_TCP_TERM_2, true);
        break;
    case PS_TCP_TERM_2:
        phase_transition(PS_TCP_TERM_3, true);
        break;
    case PS_TCP_TERM_3:
        model.set_executable(0);
        break;
    case PS_UDP_REQ:
        phase_transition(PS_UDP_REP, true);
        break;
    case PS_UDP_REP:
        model.set_executable(0);
        break;
    case PS_ICMP_ECHO_REQ:
        phase_transition(PS_ICMP_ECHO_REP, true);
        break;
    case PS_ICMP_ECHO_REP:
        model.set_executable(0);
        break;
    default:
        Logger::error("Unknown protocol state " + std::to_string(proto_state));
    }
}

void ForwardingProcess::phase_transition(uint8_t next_proto_state,
                                         bool change_direction) const {
    // check if the new src_node is a middlebox
    // if it is, do not inject packet and mark the connection as "not
    // executable" (missing packet)
    Node *new_src_node =
        change_direction ? model.get_pkt_location() : model.get_src_node();
    if (typeid(*new_src_node) == typeid(Middlebox)) {
        Logger::debug("Freeze conn " + std::to_string(model.get_conn()));
        model.set_executable(0);
        return;
    }

    int old_proto_state = model.get_proto_state();

    // compute seq and ack numbers
    if (PS_IS_TCP(old_proto_state)) {
        uint32_t seq = model.get_seq();
        uint32_t ack = model.get_ack();
        Payload *pl = model.get_payload();
        uint32_t payload_size = pl ? pl->get_size() : 0;
        if (payload_size > 0) {
            seq += payload_size;
        } else if (PS_HAS_SYN(old_proto_state) || PS_HAS_FIN(old_proto_state)) {
            seq += 1;
        }
        if (change_direction) {
            std::swap(seq, ack);
        }
        model.set_seq(seq);
        model.set_ack(ack);
    }

    model.set_proto_state(next_proto_state);

    // update src/dst IP, src node, src/dst ports, FIB, and pkt loc
    if (change_direction) {
        uint32_t src_ip = model.get_src_ip();
        EqClass *dst_ip_ec = model.get_dst_ip_ec();
        // the next src IP
        model.set_src_ip(dst_ip_ec->representative_addr().get_value());
        // the next dst IP EC
        EqClass *next_dst_ip_ec = EqClassMgr::get().find_ec(src_ip);
        model.set_dst_ip_ec(next_dst_ip_ec);
        // src node
        model.set_src_node(model.get_pkt_location());
        // src port and dst port
        uint16_t src_port = model.get_src_port();
        uint16_t dst_port = model.get_dst_port();
        model.set_src_port(dst_port);
        model.set_dst_port(src_port);
        // FIB
        model.update_fib();
    } else {
        model.set_pkt_location(model.get_src_node());
    }

    // update payload
    model.set_payload(PayloadMgr::get().get_payload_from_model());

    model.set_fwd_mode(fwd_mode::FIRST_COLLECT);
    model.set_ingress_intf(nullptr);
    model.reset_candidates();
}

void ForwardingProcess::inject_packet(Middlebox *mb) {
    Stats::set_overall_lat_t1();

    // check out current pkt_hist and rewind the middlebox state
    PacketHistory *pkt_hist = model.get_pkt_hist();
    NodePacketHistory *current_nph = pkt_hist->get_node_pkt_hist(mb);
    Stats::set_rewind_lat_t1();
    int rewind_injections = mb->rewind(current_nph);
    Stats::set_rewind_latency();
    Stats::set_rewind_injection_count(rewind_injections);

    // construct new packet
    Packet *new_pkt = new Packet(model);
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
    model.set_pkt_hist(std::move(new_pkt_hist));

    // inject packet
    Logger::info("Injecting packet: " + new_pkt->to_string());
    std::list<Packet> recv_pkts = mb->send_pkt(*new_pkt);
    update_model_from_pkts(mb, recv_pkts);

    Stats::set_overall_latency();
}

/*
 * No multicast.
 */
void ForwardingProcess::update_model_from_pkts(
    Middlebox *mb,
    std::list<Packet> &recv_pkts) const {
    int orig_conn = model.get_conn();
    bool current_conn_updated = false;

    for (Packet &recv_pkt : recv_pkts) {
        if (recv_pkt.empty()) {
            Logger::info("Received packet: (empty)");
            continue;
        }

        Net::get().identify_conn(recv_pkt);
        Net::get().process_proto_state(recv_pkt);
        Net::get().process_seq_ack(recv_pkt, mb->get_emulation());
        Logger::info("Received packet " + recv_pkt.to_string());

        if (recv_pkt.conn() == orig_conn) {
            current_conn_updated = true;
        }

        // set conn (and recover it later)
        model.set_conn(recv_pkt.conn());

        // map the packet info (5-tuple + seq/ack + payload + FIB + control
        // logic) back to the system state
        EqClass *old_dst_ip_ec = model.get_dst_ip_ec();
        model.set_executable(2);
        model.set_proto_state(recv_pkt.get_proto_state());
        model.set_src_ip(recv_pkt.get_src_ip().get_value());
        model.set_dst_ip_ec(EqClassMgr::get().find_ec(recv_pkt.get_dst_ip()));
        model.set_src_port(recv_pkt.get_src_port());
        model.set_dst_port(recv_pkt.get_dst_port());
        model.set_seq(recv_pkt.get_seq());
        model.set_ack(recv_pkt.get_ack());
        model.set_payload(recv_pkt.get_payload());
        if (old_dst_ip_ec != model.get_dst_ip_ec()) {
            model.update_fib();
        }

        // update the remaining connection state variables based on the inferred
        // connection info
        if (recv_pkt.is_new()) {
            model.set_num_conns(model.get_num_conns() + 1);
            model.set_src_node(mb);
            model.set_tx_node(mb);
            model.set_rx_node(nullptr);
            model.set_pkt_location(mb);
            model.set_fwd_mode(fwd_mode::FIRST_FORWARD);
            model.set_ingress_intf(nullptr);
            model.set_path_choices(Choices());
        } else if (recv_pkt.next_phase()) {
            if (recv_pkt.opposite_dir()) {
                model.set_src_node(mb);
            } else {
                model.set_pkt_location(model.get_src_node());
            }
            model.set_fwd_mode(fwd_mode::FIRST_FORWARD);
            model.set_ingress_intf(nullptr);
        } else { // same phase (middlebox is not an endpoint)
            assert(model.get_fwd_mode() == fwd_mode::COLLECT_NHOPS);
            model.set_fwd_mode(fwd_mode::FORWARD_PACKET);
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
        model.set_candidates(std::move(candidates));

        model.set_conn(orig_conn);
    }

    // The current connection isn't updated. I.e., no packets are received for
    // the current connection. In this case we assume the injected packet is
    // accepted by the middlebox, unless in the future (TODO) we incorporate
    // DropMon and know if the injected packet were dropped. This is useful
    // because we can assume that packets of PS_TCP_INIT_3, PS_TCP_L7_REQ_A,
    // PS_TCP_L7_REP_A in TCP are delivered even when there's no response.
    if (!current_conn_updated) {
        Logger::info("No packets are received for conn " +
                     std::to_string(model.get_conn()) +
                     ", assuming the injected packet is delivered");
        model.set_executable(2);
        model.set_fwd_mode(fwd_mode::FORWARD_PACKET);
        Candidates candidates;
        candidates.add(FIB_IPNH(mb, nullptr, mb, nullptr));
        model.set_candidates(std::move(candidates));
    }

    if (model.get_candidates()->empty()) {
        // this only happens if the current connection is updated, but no next
        // hop is available for the received outgoing packet
        Logger::info("Connection " + std::to_string(model.get_conn()) +
                     " dropped by " + mb->to_string());
        model.set_fwd_mode(fwd_mode::DROPPED);
        model.set_executable(0);
    } else {
        // update choice_count for the current connection
        model.set_choice_count(model.get_candidates()->size());
    }
}
