#include "policy/waypoint.hpp"

#include <cstring>

#include "process/forwarding.hpp"
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

void WaypointPolicy::init(State *state)
{
    state->violated = pass_through;
}

int WaypointPolicy::check_violation(State *state)
{
    int mode = state->comm_state[state->comm].fwd_mode;

    if (mode == fwd_mode::COLLECT_NHOPS || mode == fwd_mode::FIRST_COLLECT) {
        Node *current_node;
        memcpy(&current_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
        if (waypoints.count(current_node) > 0) {
            // encountering waypoint
            state->violated = !pass_through;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
