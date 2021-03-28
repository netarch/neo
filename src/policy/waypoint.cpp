#include "policy/waypoint.hpp"

#include "node.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string WaypointPolicy::to_string() const
{
    std::string ret = "waypoint " + comms[0].start_nodes_str();
    if (pass_through) {
        ret += " ---> [";
    } else {
        ret += " -X-> [";
    }
    for (Node *node : waypoints) {
        ret += " " + node->to_string();
    }
    ret += " ] ->";
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

    if (mode == fwd_mode::COLLECT_NHOPS || mode == fwd_mode::FIRST_COLLECT) {
        if (waypoints.count(get_pkt_location(state)) > 0) {
            // encountering waypoint
            state->violated = !pass_through;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
