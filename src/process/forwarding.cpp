#include "process/forwarding.hpp"
#include "plankton.hpp"

static Plankton& plankton = Plankton::get_instance();
static std::vector<Node *> selected_nodes;

ForwardingProcess::ForwardingProcess(): current_node(nullptr),
    ingress_intf(nullptr), l3_nhop(nullptr)
{
}

void ForwardingProcess::init(Node *start_node)
{
    current_node = start_node;
}

void ForwardingProcess::exec_step(State *state) const
{
    auto& next_step = state->network_state[state->itr_ec].next_step;
    int& choice_count = state->choice_count;
    state->choice_count = 0; /* No non-determinsitic choices to be made as of now */

    switch (next_step) {
        case step_type::INIT       :
            selected_nodes = plankton.get_policy()->get_packet_entry_points(state);
            assert(!selected_nodes.empty());
            choice_count = selected_nodes.size() - 1;
            next_step = int(step_type::INJECT_PACKET);
            break;
        default                    :
            throw std::logic_error("Unknown step");
    }

    return;
}
