#include "policy/one-request.hpp"

#include "node.hpp"
#include "reachcounts.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string OneRequestPolicy::to_string() const
{
    std::string ret = "OneRequest: [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void OneRequestPolicy::init(State *state, const Network *network)
{
    Policy::init(state, network);
    set_violated(state, true);
    ReachCounts reach_counts;
    set_reach_counts(state, std::move(reach_counts));
}

int OneRequestPolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);
    int proto_state = get_proto_state(state);

    if (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) {
        Node *rx_node = get_rx_node(state);
        if (target_nodes.count(rx_node)) {
            ReachCounts new_reach_counts(*get_reach_counts(state));
            new_reach_counts.increase(rx_node);
            if (new_reach_counts.total_counts() > 1) {
                set_violated(state, true); // violation
                state->choice_count = 0;
            }
            set_reach_counts(state, std::move(new_reach_counts));

            // so that the connection won't be chosen
            set_executable(state, 0);
        }
    }

    return POL_NULL;
}
