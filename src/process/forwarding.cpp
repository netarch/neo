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
    this->start_nodes = start_nodes;
    update_candidates(state, start_nodes);
    state->network_state[state->itr_ec].fwd_mode
        = int(fwd_mode::FORWARD_PACKET);
    memset(state->network_state[state->itr_ec].pkt_location, 0, sizeof(Node *));
}

void ForwardingProcess::exec_step(State *state)
{
    if (!enabled) {
        return;
    }

    auto& exec_mode = state->network_state[state->itr_ec].fwd_mode;
    switch (exec_mode) {
        case fwd_mode::FORWARD_PACKET:
            forward_packet(state);
            break;
        case fwd_mode::COLLECT_NHOPS:
            collect_next_hops(state);
            break;
        case fwd_mode::ACCEPTED:
        case fwd_mode::DROPPED:
            state->choice_count = 0;
            break;
        default:
            Logger::get_instance().err("forwarding process: unknown step");
    }
}

void ForwardingProcess::update_candidates(
    State *state,
    const std::vector<Node *>& new_candidates)
{
    std::vector<Node *> *candidates = new std::vector<Node *>(new_candidates);
    auto res = candidates_hist.insert(candidates);
    if (!res.second) {
        delete candidates;
        candidates = *(res.first);
    }
    state->choice_count = candidates->size();
    memcpy(state->candidates, &candidates, sizeof(std::vector<Node *> *));
}

void ForwardingProcess::forward_packet(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->network_state[state->itr_ec].pkt_location,
           sizeof(Node *));
    std::vector<Node *> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<Node *> *));
    Node *next_hop = (*candidates)[state->choice];
    if (next_hop == current_node) {
        Logger::get_instance().info("Packet delivered at "
                                    + next_hop->to_string());
        state->network_state[state->itr_ec].fwd_mode
            = int(fwd_mode::ACCEPTED);
        state->choice_count = 0;
        return;
    }
    memcpy(state->network_state[state->itr_ec].pkt_location,
           &next_hop, sizeof(Node *));
    Logger::get_instance().info("Packet forwarded to " + next_hop->to_string());
    state->network_state[state->itr_ec].fwd_mode
        = int(fwd_mode::COLLECT_NHOPS);
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
        state->network_state[state->itr_ec].fwd_mode
            = int(fwd_mode::DROPPED);
        state->choice_count = 0;
        return;
    }
    std::vector<Node *> l3_next_hops;
    for (const FIB_IPNH& next_hop : next_hops) {
        l3_next_hops.push_back(next_hop.get_l3_node());
    }
    update_candidates(state, l3_next_hops);
    state->network_state[state->itr_ec].fwd_mode
        = int(fwd_mode::FORWARD_PACKET);
}

size_t CandHash::operator()(const std::vector<Node *> *const& candidates) const
{
    return hash::hash(candidates->data(), candidates->size() * sizeof(Node *));
}

bool CandEq::operator()(const std::vector<Node *> *const& a,
                        const std::vector<Node *> *const& b) const
{
    if (a->size() != b->size()) {
        return false;
    }
    return memcmp(a->data(), b->data(), a->size() * sizeof(Node *)) == 0;
}
