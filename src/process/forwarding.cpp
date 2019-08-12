#include <cstring>

#include "process/forwarding.hpp"

enum exec_type {
    INIT = 0,
    FORWARD_PACKET = 1,
    COLLECT_NHOPS = 2
};

ForwardingProcess::~ForwardingProcess()
{
    for (auto& candidates : candidates_hist) {
        delete candidates;
    }
}

void ForwardingProcess::init(State *state,
                             const std::vector<Node *>& start_nodes)
{
    this->start_nodes = start_nodes;
    update_candidates(state, start_nodes);
    state->network_state[state->itr_ec].fwd_mode
        = int(exec_type::FORWARD_PACKET);
    memset(state->network_state[state->itr_ec].pkt_location, 0, sizeof(Node *));
}

void ForwardingProcess::exec_step(State *state, const Network& net)
{
    if (!enabled) {
        return;
    }

    auto& exec_mode = state->network_state[state->itr_ec].fwd_mode;
    switch (exec_mode) {
        case exec_type::INIT:
            init(state, start_nodes);
            break;
        case exec_type::FORWARD_PACKET:
            forward_packet(state);
            break;
        case exec_type::COLLECT_NHOPS:
            collect_next_hops(state, net);
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
        state->choice_count = 0;
        return;
    }
    memcpy(state->network_state[state->itr_ec].pkt_location,
           &next_hop, sizeof(Node *));
    Logger::get_instance().info("Packet forwarded to " + next_hop->to_string());
    state->network_state[state->itr_ec].fwd_mode
        = int(exec_type::COLLECT_NHOPS);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::collect_next_hops(State *state, const Network& net)
{
    Node *current_node;
    memcpy(&current_node, state->network_state[state->itr_ec].pkt_location,
           sizeof(Node *));
    const std::set<FIB_IPNH>& next_hops = net.fib_lookup(current_node);
    if (next_hops.empty()) {
        Logger::get_instance().info("Packet dropped by "
                                    + current_node->to_string());
        state->choice_count = 0;
        return;
    }
    std::vector<Node *> l3_next_hops;
    for (const FIB_IPNH& next_hop : next_hops) {
        l3_next_hops.push_back(next_hop.get_l3_node());
    }
    update_candidates(state, l3_next_hops);
    state->network_state[state->itr_ec].fwd_mode
        = int(exec_type::FORWARD_PACKET);
}

size_t CandHash::operator()(const std::vector<Node *> *const& candidates __attribute__((unused))) const
{
    return 0;
    //size_t value = 0;
    //for (Node *node : *candidates) {
    //    // TODO boost::hash_combine(value, node);
    //}
    //return value;
}

bool CandEq::operator()(const std::vector<Node *> *const& a,
                        const std::vector<Node *> *const& b) const
{
    return *a == *b;
}
