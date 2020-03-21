#include "policy/waypoint.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

WaypointPolicy::WaypointPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    auto wp_regex = config->get_as<std::string>("waypoint");
    auto through = config->get_as<bool>("pass_through");
    auto comm_cfg = config->get_table("communication");

    if (!wp_regex) {
        Logger::get().err("Missing waypoint");
    }
    if (!through) {
        Logger::get().err("Missing pass_through");
    }
    if (!comm_cfg) {
        Logger::get().err("Missing communication");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*wp_regex))) {
            waypoints.insert(node.second);
        }
    }

    pass_through = *through;

    Communication comm(comm_cfg, net);
    comms.push_back(std::move(comm));
}

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
