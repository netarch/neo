#include "process/forwarding.hpp"

#include <cstring>
#include <typeinfo>

#include "stats.hpp"
#include "lib/logger.hpp"
#include "lib/hash.hpp"
#include "choices.hpp"
#include "model.h"

ForwardingProcess::~ForwardingProcess()
{
    for (auto& candidates : candidates_hist) {
        delete candidates;
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
        Logger::get().err("Unknown protocol: "
                          + std::to_string(policy->get_protocol(state)));
    }
    state->comm_state[state->comm].pkt_state = pkt_state;
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::PACKET_ENTRY);
    memset(state->comm_state[state->comm].seq, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].ack, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_ip, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
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

    // initialize packet EC and update the FIB
    EqClass *ec = policy->get_initial_ec(state);
    memcpy(state->comm_state[state->comm].ec, &ec, sizeof(EqClass *));
    Logger::get().info("EC: " + ec->to_string());
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
            Logger::get().err("Forwarding process: unknown mode ("
                              + std::to_string(mode) + ")");
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
    // TODO: better logging information
    Logger::get().info("Packet (state: " + std::to_string(pkt_state)
                       + ") started at " + entry->to_string());
    if (PS_IS_FIRST(pkt_state)) {
        policy->set_comm_tx(state, entry);
    }
}

void ForwardingProcess::first_collect(State *state)
{
    collect_next_hops(state);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward(State *state) const
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

void ForwardingProcess::collect_next_hops(State *state)
{
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
        next_hops = inject_packet(state, (Middlebox *)current_node);
    }

    if (next_hops.empty()) {
        dropped(state);
        return;
    }

    std::vector<FIB_IPNH> candidates;
    if (next_hops.size() > 1) { //multipath
        //first check if a choice was made previously
        auto pastChoice = getChoiceIfPresent(state);
        if (pastChoice) {
            candidates.push_back(pastChoice.value());
        }
    }

    if (candidates.empty()) {
        for (const FIB_IPNH& next_hop : next_hops) {
            candidates.push_back(next_hop);
        }
    }

    update_candidates(state, candidates);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FORWARD_PACKET);
}

void ForwardingProcess::forward_packet(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    setChoice(state, (*candidates)[state->choice]);
    Node *next_hop = (*candidates)[state->choice].get_l3_node();
    if (next_hop == current_node) {
        // check if the endpoints remain consistent
        int pkt_state = state->comm_state[state->comm].pkt_state;
        Node *current_node;
        memcpy(&current_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
        if (PS_IS_FIRST(pkt_state)) {
            // store the original receiving endpoint of the communication
            policy->set_comm_rx(state, current_node);
        } else if ((PS_IS_REQUEST(pkt_state)
                    && current_node != policy->get_comm_rx(state))
                   || (PS_IS_REPLY(pkt_state)
                       && current_node != policy->get_comm_tx(state))) {
            dropped(state);
            return;
        }

        Logger::get().info("Packet delivered at " + next_hop->to_string());
        state->comm_state[state->comm].fwd_mode = int(fwd_mode::ACCEPTED);
        state->choice_count = 1;
        return;
    }
    memcpy(state->comm_state[state->comm].pkt_location, &next_hop,
           sizeof(Node *));

    Interface *ingress_intf = (*candidates)[state->choice].get_l3_intf();
    memcpy(state->comm_state[state->comm].ingress_intf, &ingress_intf,
           sizeof(Interface *));

    Logger::get().info("Packet forwarded to " + next_hop->to_string() + ", "
                       + ingress_intf->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::COLLECT_NHOPS);
    state->choice_count = 1;    // deterministic choice
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
        seq ^= ack ^= seq ^= ack;   // swap
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
        EqClass *next_ec = policy->get_ecs(state).find_ec(src_ip);
        memcpy(state->comm_state[state->comm].ec, &next_ec, sizeof(EqClass *));
        Logger::get().info("EC: " + next_ec->to_string());

        // update FIB according to the new EC
        network.update_fib(state);
    }

    state->comm_state[state->comm].pkt_state = next_pkt_state;
    state->comm_state[state->comm].fwd_mode = fwd_mode::PACKET_ENTRY;
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));
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
            Logger::get().err("Forwarding process: unknown packet state "
                              + std::to_string(pkt_state));
    }
}

void ForwardingProcess::dropped(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));
    Logger::get().info("Packet dropped by " + current_node->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::DROPPED);
    state->choice_count = 0;
}

std::set<FIB_IPNH> ForwardingProcess::inject_packet(State *state, Middlebox *mb)
{
    Stats::get().set_overall_lat_t1();

    // check state's pkt_hist and rewind the middlebox state
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->comm_state[state->comm].pkt_hist,
           sizeof(PacketHistory *));
    NodePacketHistory *current_nph = pkt_hist->get_node_pkt_hist(mb);
    Stats::get().set_rewind_lat_t1();
    int rewind_injections = mb->rewind(current_nph);
    Stats::get().set_rewind_latency();
    Stats::get().set_rewind_injection_count(rewind_injections);

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
    Stats::get().set_pkt_lat_t1();
    std::set<FIB_IPNH> next_hops = mb->send_pkt(*new_pkt);
    Stats::get().set_pkt_latency();

    Stats::get().set_overall_latency();
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
