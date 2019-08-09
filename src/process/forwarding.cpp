#include "process/forwarding.hpp"

ForwardingProcess::ForwardingProcess(): current_node(nullptr),
    ingress_intf(nullptr), l3_nhop(nullptr)
{
}

void ForwardingProcess::init(State *state,
                             const std::vector<Node *>& start_nodes)
{
    this->start_nodes = start_nodes;
    // non-deterministically choose a node from start_nodes
    state->choice_count = start_nodes.size();
    Logger::get_instance().info("Choice count: "
                                + std::to_string(state->choice_count));

    auto& exec_mode = state->network_state[state->itr_ec].exec_mode;
    exec_mode = int(exec_type::INJECT_PACKET);
}

void ForwardingProcess::exec_step(State *state) const
{
    auto& exec_mode = state->network_state[state->itr_ec].exec_mode;
    state->choice_count = 0;    // no non-determinsitic choices to be made

    switch (exec_mode) {
        case exec_type::INJECT_PACKET:
            Logger::get_instance().info("Chosen injection point is: "
                                        + std::to_string(state->choice));
            break;
        default:
            Logger::get_instance().err("Unknown step");
    }
}
