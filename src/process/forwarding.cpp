#include <cstring>

#include "process/forwarding.hpp"
#include "fib.hpp"
#include "lib/hash.hpp"

ForwardingProcess::~ForwardingProcess()
{
    for (auto& candidates : candidates_hist) {
        delete candidates;
    }
}

void ForwardingProcess::config(State *state,
                               const std::vector<Node *>& start_nodes)
{
    std::vector<FIB_IPNH> candidates;
    for (Node *start_node : start_nodes) {
        candidates.push_back(FIB_IPNH(start_node, nullptr, start_node, nullptr));
    }
    update_candidates(state, candidates);
    state->network_state[state->itr_ec].fwd_mode
        = int(fwd_mode::INJECT_PACKET);
    memset(state->network_state[state->itr_ec].src_addr, 0, sizeof(uint32_t));
    memset(state->network_state[state->itr_ec].src_node, 0, sizeof(Node *));
    memset(state->network_state[state->itr_ec].pkt_location, 0, sizeof(Node *));
}

void ForwardingProcess::exec_step(State *state)
{
    if (!enabled) {
        return;
    }

    auto& mode = state->network_state[state->itr_ec].fwd_mode;
    switch (mode) {
        case fwd_mode::INJECT_PACKET:
            inject_packet(state);
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
        case fwd_mode::DROPPED:
            state->choice_count = 0;
            break;
        default:
            Logger::get_instance().err("forwarding process: unknown mode ("
                                       + std::to_string(mode) + ")");
    }
}

void ForwardingProcess::inject_packet(State *state) const
{
    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    Node *entry = (*candidates)[state->choice].get_l3_node();
    memcpy(state->network_state[state->itr_ec].src_node, &entry,
           sizeof(Node *));
    memcpy(state->network_state[state->itr_ec].pkt_location, &entry,
           sizeof(Node *));
    memset(state->network_state[state->itr_ec].ingress_intf, 0,
           sizeof(Interface *));
    Logger::get_instance().info("Packet injected at " + entry->to_string());
    state->network_state[state->itr_ec].fwd_mode = int(fwd_mode::FIRST_COLLECT);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::first_collect(State *state)
{
    collect_next_hops(state);
    state->network_state[state->itr_ec].fwd_mode = int(fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward(State *state) const
{
    // update the source address
    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    FIB_IPNH next_hop = (*candidates)[state->choice];
    if (next_hop.get_l2_intf()) {
        const Interface *egress_intf
            = next_hop.get_l2_node()->get_peer(
                  next_hop.get_l2_intf()->get_name()
              ).second;
        uint32_t src_addr_value = egress_intf->addr().get_value();
        memcpy(state->network_state[state->itr_ec].src_addr, &src_addr_value,
               sizeof(uint32_t));
    }

    forward_packet(state);
}

void ForwardingProcess::forward_packet(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->network_state[state->itr_ec].pkt_location,
           sizeof(Node *));

    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));

    Node *next_hop = (*candidates)[state->choice].get_l3_node();
    if (next_hop == current_node) {
        Logger::get_instance().info("Packet delivered at "
                                    + next_hop->to_string());
        state->network_state[state->itr_ec].fwd_mode = int(fwd_mode::ACCEPTED);
        state->choice_count = 0;
        return;
    }
    memcpy(state->network_state[state->itr_ec].pkt_location, &next_hop,
           sizeof(Node *));

    Interface *ingress_intf = (*candidates)[state->choice].get_l3_intf();
    memcpy(state->network_state[state->itr_ec].ingress_intf, &ingress_intf,
           sizeof(Interface *));

    Logger::get_instance().info("Packet forwarded to " + next_hop->to_string()
                                + ", " + ingress_intf->to_string());
    state->network_state[state->itr_ec].fwd_mode = int(fwd_mode::COLLECT_NHOPS);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::collect_next_hops(State *state)
{
    Node *current_node;
    memcpy(&current_node, state->network_state[state->itr_ec].pkt_location,
           sizeof(Node *));
    FIB *fib;
    memcpy(&fib, state->network_state[state->itr_ec].fib, sizeof(FIB *));
    const std::set<FIB_IPNH>& next_hops = fib->lookup(current_node);
    if (next_hops.empty()) {
        Logger::get_instance().info("Packet dropped by "
                                    + current_node->to_string());
        state->network_state[state->itr_ec].fwd_mode = int(fwd_mode::DROPPED);
        state->choice_count = 0;
        return;
    }
    std::vector<FIB_IPNH> candidates;
    for (const FIB_IPNH& next_hop : next_hops) {
        candidates.push_back(next_hop);
    }
    update_candidates(state, candidates);
    state->network_state[state->itr_ec].fwd_mode
        = int(fwd_mode::FORWARD_PACKET);
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
