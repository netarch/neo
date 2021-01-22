#include "process/forwarding.hpp"

#include <libnet.h>
#include <cstring>
#include <typeinfo>
#include <utility>

#include "stats.hpp"
#include "lib/logger.hpp"
#include "lib/hash.hpp"
#include "model.h"

std::string fwd_mode_to_str(int mode)
{
    switch (mode) {
        case fwd_mode::PACKET_ENTRY:
            return "fwd_mode::PACKET_ENTRY";
        case fwd_mode::FIRST_COLLECT:
            return "fwd_mode::FIRST_COLLECT";
        case fwd_mode::FIRST_FORWARD:
            return "fwd_mode::FIRST_FORWARD";
        case fwd_mode::COLLECT_NHOPS:
            return "fwd_mode::COLLECT_NHOPS";
        case fwd_mode::FORWARD_PACKET:
            return "fwd_mode::FORWARD_PACKET";
        case fwd_mode::ACCEPTED:
            return "fwd_mode::ACCEPTED";
        case fwd_mode::DROPPED:
            return "fwd_mode::DROPPED";
        default:
            Logger::error("Unknown forwarding mode: " + std::to_string(mode));
            return "";
    }
}

ForwardingProcess::~ForwardingProcess()
{
    for (auto& candidates : candidates_hist) {
        delete candidates;
    }
    for (auto& choices : choices_hist) {
        delete choices;
    }
    for (auto& packet : all_pkts) {
        delete packet;
    }
    for (auto& node_pkt_hist : node_pkt_hist_hist) {
        delete node_pkt_hist;
    }
    for (auto& pkt_hist : pkt_hist_hist) {
        delete pkt_hist;
    }
}

void ForwardingProcess::init(State *state, Network& network, Policy *policy)
{
    this->policy = policy;

    uint8_t pkt_state = 0;
    if (policy->get_protocol(state) == proto::PR_HTTP) {
        pkt_state = PS_TCP_INIT_1;
    } else if (policy->get_protocol(state) == proto::PR_ICMP_ECHO) {
        pkt_state = PS_ICMP_ECHO_REQ;
    } else {
        Logger::error("Unknown protocol: "
                      + std::to_string(policy->get_protocol(state)));
    }
    state->comm_state[state->comm].pkt_state = pkt_state;
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::PACKET_ENTRY);
    memset(state->comm_state[state->comm].seq, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].ack, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_ip, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].tx_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].rx_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].pkt_location, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));

    // initialize packet history
    PacketHistory *pkt_hist = new PacketHistory(network);
    auto res = pkt_hist_hist.insert(pkt_hist);
    if (!res.second) {
        delete pkt_hist;
        pkt_hist = *(res.first);
    }
    memcpy(state->comm_state[state->comm].pkt_hist, &pkt_hist,
           sizeof(PacketHistory *));

    // update candidates as start nodes
    std::vector<FIB_IPNH> candidates;
    for (Node *start_node : policy->get_start_nodes(state)) {
        candidates.push_back(FIB_IPNH(start_node, nullptr, start_node,
                                      nullptr));
    }
    update_candidates(state, candidates);

    // update choices as an empty map
    update_choices(state, Choices());

    // initialize packet EC and update the FIB
    EqClass *ec = policy->get_initial_ec(state);
    memcpy(state->comm_state[state->comm].ec, &ec, sizeof(EqClass *));
    Logger::info("EC: " + ec->to_string());
    network.update_fib(state);
}

void ForwardingProcess::reset(State *state, Network& network, Policy *policy)
{
    if (!enabled) {
        return;
    }

    uint8_t pkt_state = 0;
    if (policy->get_protocol(state) == proto::PR_HTTP) {
        pkt_state = PS_TCP_INIT_1;
    } else if (policy->get_protocol(state) == proto::PR_ICMP_ECHO) {
        pkt_state = PS_ICMP_ECHO_REQ;
    } else {
        Logger::error("Unknown protocol: "
                      + std::to_string(policy->get_protocol(state)));
    }
    state->comm_state[state->comm].pkt_state = pkt_state;
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::PACKET_ENTRY);
    memset(state->comm_state[state->comm].seq, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].ack, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_ip, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].tx_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].rx_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].pkt_location, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));

    // update candidates as start nodes
    std::vector<FIB_IPNH> candidates;
    for (Node *start_node : policy->get_start_nodes(state)) {
        candidates.push_back(FIB_IPNH(start_node, nullptr, start_node,
                                      nullptr));
    }
    update_candidates(state, candidates);

    // initialize packet EC and update the FIB
    EqClass *ec = policy->get_initial_ec(state);
    memcpy(state->comm_state[state->comm].ec, &ec, sizeof(EqClass *));
    Logger::info("EC: " + ec->to_string());
    network.update_fib(state);
}

void ForwardingProcess::exec_step(State *state, Network& network)
{
    if (!enabled) {
        return;
    }

    int mode = state->comm_state[state->comm].fwd_mode;
    switch (mode) {
        case fwd_mode::PACKET_ENTRY:
            packet_entry(state);
            break;
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
            state->choice_count = 0;
            break;
        default:
            Logger::error("Unknown forwarding mode: " + std::to_string(mode));
    }
}

void ForwardingProcess::packet_entry(State *state) const
{
    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    Node *entry = (*candidates)[state->choice].get_l3_node();
    memcpy(state->comm_state[state->comm].src_node, &entry, sizeof(Node *));
    memcpy(state->comm_state[state->comm].pkt_location, &entry, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FIRST_COLLECT);
    state->choice_count = 1;    // deterministic choice

    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;
    Logger::info("Packet (state: " + std::to_string(pkt_state) + ") started at "
                 + entry->to_string());
    if (PS_IS_FIRST(pkt_state)) {
        memcpy(state->comm_state[state->comm].tx_node, &entry, sizeof(Node *));
    }
}

void ForwardingProcess::first_collect(State *state, Network& network)
{
    collect_next_hops(state, network);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward(State *state)
{
    int pkt_state = state->comm_state[state->comm].pkt_state;
    if (PS_IS_FIRST(pkt_state)) {
        // update the source IP address according to the egress interface
        std::vector<FIB_IPNH> *candidates;
        memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
        FIB_IPNH next_hop = (*candidates)[state->choice];
        if (next_hop.get_l2_intf()) {
            const Interface *egress_intf
                = next_hop.get_l2_node()->get_peer(
                      next_hop.get_l2_intf()->get_name()
                  ).second;
            uint32_t src_ip = egress_intf->addr().get_value();
            memcpy(state->comm_state[state->comm].src_ip, &src_ip,
                   sizeof(uint32_t));
        }
    }

    forward_packet(state);
}

void ForwardingProcess::collect_next_hops(State *state, Network& network)
{
    // TODO: std::vector
    std::set<FIB_IPNH> next_hops;

    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    if (typeid(*current_node) == typeid(Node)) {
        // current_node is a Node; look up next hops from the fib
        FIB *fib;
        memcpy(&fib, state->comm_state[state->comm].fib, sizeof(FIB *));
        next_hops = fib->lookup(current_node);
    } else {
        // current_node is a Middlebox; inject packet to get next hops
        next_hops = inject_packet(state, (Middlebox *)current_node, network);
    }

    if (next_hops.empty()) {
        dropped(state);
        return;
    }

    std::vector<FIB_IPNH> candidates;
    std::optional<FIB_IPNH> choice;

    // in case of multipath, use the past choice if it's been made
    if (next_hops.size() > 1 && (choice = get_choice(state))) {
        candidates.push_back(choice.value());
    } else {
        for (const FIB_IPNH& next_hop : next_hops) {
            candidates.push_back(next_hop);
        }
    }

    update_candidates(state, candidates);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FORWARD_PACKET);
}

void ForwardingProcess::forward_packet(State *state)
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));

    // in case of multipath, remember the path choice
    if (candidates->size() > 1) {
        add_choice(state, (*candidates)[state->choice]);
    }

    Node *next_hop = (*candidates)[state->choice].get_l3_node();
    if (next_hop == current_node) {
        // check if the endpoints remain consistent
        int pkt_state = state->comm_state[state->comm].pkt_state;
        Node *current_node, *tx_node, *rx_node;
        memcpy(&current_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
        memcpy(&tx_node, state->comm_state[state->comm].tx_node,
               sizeof(Node *));
        memcpy(&rx_node, state->comm_state[state->comm].rx_node,
               sizeof(Node *));
        if (PS_IS_FIRST(pkt_state)) {
            // store the original receiving endpoint of the communication
            memcpy(state->comm_state[state->comm].rx_node, &current_node,
                   sizeof(Node *));
        } else if (PS_IS_REQUEST(pkt_state) && current_node != rx_node) {
            Logger::info("current_node: " + current_node->to_string());
            Logger::info("rx_node: " + rx_node->to_string());
            Logger::warn("Inconsistent endpoints");
            dropped(state);
            return;
        } else if (PS_IS_REPLY(pkt_state) && current_node != tx_node) {
            Logger::info("current_node: " + current_node->to_string());
            Logger::info("tx_node: " + tx_node->to_string());
            Logger::warn("Inconsistent endpoints");
            dropped(state);
            return;
        }

        Logger::info("Packet delivered at " + next_hop->to_string());
        state->comm_state[state->comm].fwd_mode = int(fwd_mode::ACCEPTED);
        state->choice_count = 1;
        return;
    }
    memcpy(state->comm_state[state->comm].pkt_location, &next_hop,
           sizeof(Node *));

    Interface *ingress_intf = (*candidates)[state->choice].get_l3_intf();
    memcpy(state->comm_state[state->comm].ingress_intf, &ingress_intf,
           sizeof(Interface *));

    Logger::info("Packet forwarded to " + next_hop->to_string() + ", "
                 + ingress_intf->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::COLLECT_NHOPS);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::accepted(State *state, Network& network)
{
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;
    switch (pkt_state) {
        case PS_TCP_INIT_1:
            phase_transition(state, network, PS_TCP_INIT_2, true);
            break;
        case PS_TCP_INIT_2:
            phase_transition(state, network, PS_TCP_INIT_3, true);
            break;
        case PS_TCP_INIT_3:
            phase_transition(state, network, PS_HTTP_REQ, false);
            break;
        case PS_HTTP_REQ:
            phase_transition(state, network, PS_HTTP_REQ_A, true);
            break;
        case PS_HTTP_REQ_A:
            phase_transition(state, network, PS_HTTP_REP, false);
            break;
        case PS_HTTP_REP:
            phase_transition(state, network, PS_HTTP_REP_A, true);
            break;
        case PS_HTTP_REP_A:
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
        case PS_ICMP_ECHO_REQ:
            phase_transition(state, network, PS_ICMP_ECHO_REP, true);
            break;
        case PS_ICMP_ECHO_REP:
            state->choice_count = 0;
            break;
        default:
            Logger::error("Forwarding process: unknown packet state "
                          + std::to_string(pkt_state));
    }
}

void ForwardingProcess::dropped(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));
    Logger::info("Packet dropped by " + current_node->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::DROPPED);
    state->choice_count = 0;
}

void ForwardingProcess::phase_transition(State *state, Network& network,
        uint8_t next_pkt_state, bool change_direction)
{
    int pkt_state = state->comm_state[state->comm].pkt_state;

    // compute seq and ack numbers
    if (PS_IS_TCP(pkt_state)) {
        uint32_t seq, ack;
        memcpy(&seq, state->comm_state[state->comm].seq, sizeof(uint32_t));
        memcpy(&ack, state->comm_state[state->comm].ack, sizeof(uint32_t));
        uint32_t payload_size = PayloadMgr::get().get_payload(state,
                                policy->get_dst_port(state))->get_size();
        if (payload_size > 0) {
            seq += payload_size;
        } else if (PS_HAS_SYN(pkt_state) || PS_HAS_FIN(pkt_state)) {
            seq += 1;
        }
        if (change_direction) {
            std::swap(seq, ack);
        }
        memcpy(state->comm_state[state->comm].seq, &seq, sizeof(uint32_t));
        memcpy(state->comm_state[state->comm].ack, &ack, sizeof(uint32_t));
    }

    // update candidates as the start node
    Node *start_node;
    if (change_direction) {
        memcpy(&start_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
    } else {
        memcpy(&start_node, state->comm_state[state->comm].src_node,
               sizeof(Node *));
    }
    std::vector<FIB_IPNH> candidates(
        1, FIB_IPNH(start_node, nullptr, start_node, nullptr));
    update_candidates(state, candidates);

    // update src IP, packet EC and the FIB
    if (change_direction) {
        uint32_t src_ip;
        memcpy(&src_ip, state->comm_state[state->comm].src_ip, sizeof(uint32_t));
        EqClass *old_ec;
        memcpy(&old_ec, state->comm_state[state->comm].ec, sizeof(EqClass *));

        // set the next src IP
        uint32_t next_src_ip = old_ec->representative_addr().get_value();
        memcpy(state->comm_state[state->comm].src_ip, &next_src_ip,
               sizeof(uint32_t));

        // set the next EC
        if (PS_IS_FIRST(pkt_state)) {
            policy->add_ec(state, src_ip);
        }
        EqClass *next_ec = policy->find_ec(state, src_ip);
        memcpy(state->comm_state[state->comm].ec, &next_ec, sizeof(EqClass *));
        Logger::info("EC: " + next_ec->to_string());

        // update FIB according to the new EC
        network.update_fib(state);
    }

    state->comm_state[state->comm].pkt_state = next_pkt_state;
    state->comm_state[state->comm].fwd_mode = fwd_mode::PACKET_ENTRY;
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));
}

bool ForwardingProcess::same_dir_comm(State *state, int comm,
                                      const Packet& pkt) const
{
    EqClass *ec;
    memcpy(&ec, state->comm_state[comm].ec, sizeof(EqClass *));
    uint32_t src_ip;
    memcpy(&src_ip, state->comm_state[comm].src_ip, sizeof(uint32_t));

    if (ec->contains(pkt.get_dst_ip()) && pkt.get_src_ip() == src_ip &&
            pkt.get_src_port() == policy->get_src_port(state) &&
            pkt.get_dst_port() == policy->get_dst_port(state)) {
        return true;
    }
    return false;
}

bool ForwardingProcess::opposite_dir_comm(State *state, int comm,
        const Packet& pkt) const
{
    EqClass *ec;
    memcpy(&ec, state->comm_state[comm].ec, sizeof(EqClass *));
    uint32_t src_ip;
    memcpy(&src_ip, state->comm_state[comm].src_ip, sizeof(uint32_t));

    if (ec->contains(pkt.get_src_ip()) && pkt.get_dst_ip() == src_ip &&
            pkt.get_dst_port() == policy->get_src_port(state) &&
            pkt.get_src_port() == policy->get_dst_port(state)) {
        return true;
    }
    return false;
}

int ForwardingProcess::find_comm(State *state, const Packet& pkt) const
{
    for (size_t comm = 0; comm < policy->num_simul_comms(); ++comm) {
        if (same_dir_comm(state, comm, pkt) ||
                opposite_dir_comm(state, comm, pkt)) {
            return comm;
        }
    }
    return -1;  // not found
}

void ForwardingProcess::convert_tcp_flags(State *state, int comm, Packet& pkt)
const
{
    if (pkt.get_pkt_state() & 0x80U) {
        uint8_t pkt_state = 0, flags = pkt.get_pkt_state() & (~0x80U);
        uint8_t old_pkt_state = state->comm_state[comm].pkt_state;
        if (flags == TH_SYN) {
            pkt_state = PS_TCP_INIT_1;
        } else if (flags == (TH_SYN | TH_ACK)) {
            pkt_state = PS_TCP_INIT_2;
        } else if (flags == TH_ACK) {
            switch (old_pkt_state) {
                case PS_TCP_INIT_2:
                case PS_HTTP_REQ:
                case PS_HTTP_REP:
                case PS_TCP_TERM_2:
                    pkt_state = old_pkt_state + 1;
                    break;
                case PS_TCP_INIT_3:
                case PS_HTTP_REQ_A:
                case PS_HTTP_REP_A:
                case PS_TCP_TERM_3:
                    pkt_state = old_pkt_state;
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_PUSH | TH_ACK)) {
            switch (old_pkt_state) {
                case PS_TCP_INIT_3:
                case PS_HTTP_REQ_A:
                    pkt_state = old_pkt_state + 1;
                    break;
                case PS_HTTP_REQ:
                case PS_HTTP_REP:
                    pkt_state = old_pkt_state;
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_FIN | TH_ACK)) {
            bool same_direction = same_dir_comm(state, comm, pkt);
            if (same_direction) {
                pkt_state = old_pkt_state;
            } else {
                pkt_state = old_pkt_state + 1;
            }
            switch (old_pkt_state) {
                case PS_HTTP_REP_A:
                case PS_TCP_TERM_1:
                case PS_TCP_TERM_2:
                case PS_TCP_TERM_3:
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else {
            Logger::error("Invalid TCP flags: " + std::to_string(flags));
        }
        pkt.set_pkt_state(pkt_state);
    }
}

std::set<FIB_IPNH> ForwardingProcess::inject_packet(State *state, Middlebox *mb,
        Network& network)
{
    Stats::set_overall_lat_t1();

    // check state's pkt_hist and rewind the middlebox state
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->comm_state[state->comm].pkt_hist,
           sizeof(PacketHistory *));
    NodePacketHistory *current_nph = pkt_hist->get_node_pkt_hist(mb);
    Stats::set_rewind_lat_t1();
    int rewind_injections = mb->rewind(current_nph);
    Stats::set_rewind_latency();
    Stats::set_rewind_injection_count(rewind_injections);

    // construct new packet
    Packet *new_pkt = new Packet(state, policy);
    auto res1 = all_pkts.insert(new_pkt);
    if (!res1.second) {
        delete new_pkt;
        new_pkt = *(res1.first);
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
    PacketHistory *new_pkt_hist = new PacketHistory(*pkt_hist);
    new_pkt_hist->set_node_pkt_hist(mb, new_nph);
    auto res3 = pkt_hist_hist.insert(new_pkt_hist);
    if (!res3.second) {
        delete new_pkt_hist;
        new_pkt_hist = *(res3.first);
    }
    memcpy(state->comm_state[state->comm].pkt_hist, &new_pkt_hist,
           sizeof(PacketHistory *));

    // inject packet
    std::vector<Packet> recv_pkts = mb->send_pkt(*new_pkt);
    //Logger::info("Received packet " + recv_pkt.to_string());

    std::set<FIB_IPNH> next_hops;

    // TODO: how to handle multiple packets
    // DEBUG
    for (Packet& recv_pkt : recv_pkts) {
        Logger::info("Received packets:");
        Logger::info("recv_pkt: " + recv_pkt.to_string());
        Logger::info("");
    }

    for (Packet& recv_pkt : recv_pkts) {

        // if the packet is not dropped, process it and continue
        if (!recv_pkt.empty()) {
            // find the next hop
            auto l2nh = mb->get_peer(recv_pkt.get_intf()->get_name());  // L2 nhop
            if (l2nh.first) {   // if the interface is truly connected
                if (!l2nh.second->is_l2()) {
                    // L2 nhop == L3 nhop
                    next_hops.insert(FIB_IPNH(l2nh.first, l2nh.second,
                                              l2nh.first, l2nh.second));
                } else {
                    L2_LAN *l2_lan = l2nh.first->get_l2lan(l2nh.second);
                    auto l3nh = l2_lan->find_l3_endpoint(recv_pkt.get_dst_ip());
                    if (l3nh.first) {
                        next_hops.insert(FIB_IPNH(l3nh.first, l3nh.second,
                                                  l2nh.first, l2nh.second));
                    }
                }
            }

            int comm = find_comm(state, recv_pkt);
            if (comm == -1) {
                /* create a new communication (and then update comm) */

                // convert the TCP flags to real pkt_state
                if (recv_pkt.get_pkt_state() & 0x80U) {
                    uint8_t flags = recv_pkt.get_pkt_state() & (~0x80U);
                    if (flags != TH_SYN) {
                        // it has to be a SYN packet for TCP
                        Logger::error("Unrecognized packet: " + recv_pkt.to_string());
                    }
                    recv_pkt.set_pkt_state(PS_TCP_INIT_1);
                }

                if (!PS_IS_FIRST(recv_pkt.get_pkt_state())) {
                    Logger::error("Unrecognized packet: " + recv_pkt.to_string());
                }

                // TODO (create a new communication)
                Logger::info("new communication");
            }

            // convert the TCP flags to real pkt_state if needed
            convert_tcp_flags(state, comm, recv_pkt);
            Logger::info("Received packet " + recv_pkt.to_string());

            if (comm == state->comm) {
                // same communication

                // next phase
                if (PS_IS_NEXT(recv_pkt.get_pkt_state(),
                               new_pkt->get_pkt_state())) {
                    std::vector<FIB_IPNH> candidates(
                        1, FIB_IPNH(mb, nullptr, mb, nullptr));
                    update_candidates(state, candidates);
                    if (state->comm_state[state->comm].fwd_mode
                            == fwd_mode::FIRST_COLLECT) {
                        state->comm_state[state->comm].fwd_mode
                            = int(fwd_mode::FIRST_FORWARD);
                    } else {
                        state->comm_state[state->comm].fwd_mode
                            = int(fwd_mode::FORWARD_PACKET);
                    }
                    exec_step(state, network);
                    exec_step(state, network);  /* should be accepted by now */

                    // update seq and ack according to the received packet
                    if (PS_HAS_SYN(recv_pkt.get_pkt_state())) {
                        uint32_t seq = recv_pkt.get_seq(), ack = recv_pkt.get_ack();
                        memcpy(state->comm_state[state->comm].seq, &seq,
                               sizeof(uint32_t));
                        memcpy(state->comm_state[state->comm].ack, &ack,
                               sizeof(uint32_t));
                    }

                    // update src_node and ingress_intf (as in packet_entry)
                    memcpy(state->comm_state[state->comm].src_node, mb,
                           sizeof(Node *));
                    Interface *ingress_intf = recv_pkt.get_intf();
                    memcpy(state->comm_state[state->comm].ingress_intf,
                           &ingress_intf, sizeof(Interface *));

                } else if (!eq_except_intf(recv_pkt, *new_pkt)) { // DEBUG/TEST
                    Logger::error("Different packet");
                }
            } else {
                // existing communication; change state->comm?

                /*
                // NAT (network address translation)
                // NOTE: we only handle the destination IP rewriting for now.
                EqClass *ec;
                memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
                if (!ec->contains(recv_pkt.get_dst_ip())) {
                    // set the next EC
                    policy->add_ec(state, recv_pkt.get_dst_ip());
                    EqClass *next_ec = policy->find_ec(state, recv_pkt.get_dst_ip());
                    memcpy(state->comm_state[state->comm].ec, &next_ec,
                           sizeof(EqClass *));
                    Logger::info("NAT EC: " + next_ec->to_string());
                    // update FIB according to the new EC
                    network.update_fib(state);
                }
                */
            }
        } else {
            // if the received packet is empty, assume it's accepted if it's ACK
            if (new_pkt->get_pkt_state() == PS_TCP_INIT_3) {
                next_hops.insert(FIB_IPNH(mb, nullptr, mb, nullptr));
            }
        }
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

void ForwardingProcess::update_candidates(
    State *state,
    const std::vector<FIB_IPNH>& new_candidates)
{
    std::vector<FIB_IPNH> *candidates
        = new std::vector<FIB_IPNH>(new_candidates);
    auto res = candidates_hist.insert(candidates);
    if (!res.second) {
        delete candidates;
        candidates = *(res.first);
    }
    state->choice_count = candidates->size();
    memcpy(state->candidates, &candidates, sizeof(std::vector<FIB_IPNH> *));
}

void ForwardingProcess::update_choices(State *state, Choices&& new_choices)
{
    Choices *choices = new Choices(new_choices);
    auto res = choices_hist.insert(choices);
    if (!res.second) {
        delete choices;
        choices = *(res.first);
    }
    memcpy(state->comm_state[state->comm].path_choices, &choices,
           sizeof(Choices *));
}

void ForwardingProcess::add_choice(State *state, const FIB_IPNH& next_hop)
{
    Choices *choices;
    memcpy(&choices, state->comm_state[state->comm].path_choices,
           sizeof(Choices *));
    EqClass *ec;
    memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    Choices new_choices(*choices);
    new_choices.add_choice(ec, current_node, next_hop);
    update_choices(state, std::move(new_choices));
}

std::optional<FIB_IPNH> ForwardingProcess::get_choice(State *state)
{
    Choices *choices;
    memcpy(&choices, state->comm_state[state->comm].path_choices,
           sizeof(Choices *));
    EqClass *ec;
    memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));
    return choices->get_choice(ec, current_node);
}

/******************************************************************************/

size_t CandHash::operator()(const std::vector<FIB_IPNH> *const& c) const
{
    return hash::hash(c->data(), c->size() * sizeof(FIB_IPNH));
}

bool CandEq::operator()(const std::vector<FIB_IPNH> *const& a,
                        const std::vector<FIB_IPNH> *const& b) const
{
    if (a->size() != b->size()) {
        return false;
    }
    return memcmp(a->data(), b->data(), a->size() * sizeof(FIB_IPNH)) == 0;
}
