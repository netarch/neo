#include "policy/waypoint.hpp"

#include "node.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string WaypointPolicy::to_string() const
{
    std::string ret = "Waypoint:\n"
        "\tpass_through: " + std::to_string(pass_through) + "\n"
        "\ttarget_nodes: [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]\n\t" + conns_str();
    return ret;
}

void WaypointPolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, false);
}

int WaypointPolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);

    if (mode == fwd_mode::COLLECT_NHOPS) {
        if (target_nodes.count(get_pkt_location(state)) > 0) {
            // encountering waypoint
            state->violated = !pass_through;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
