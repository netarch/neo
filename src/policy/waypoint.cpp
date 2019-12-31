#include "policy/waypoint.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "pan.h"

WaypointPolicy::WaypointPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config, net)
{
    auto wp_regex = config->get_as<std::string>("waypoint");
    auto through = config->get_as<bool>("pass_through");

    if (!wp_regex) {
        Logger::get().err("Missing waypoint");
    }
    if (!through) {
        Logger::get().err("Missing pass_through");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*wp_regex))) {
            waypoints.insert(node.second);
        }
    }

    pass_through = *through;
}

std::string WaypointPolicy::to_string() const
{
    std::string ret = "waypoint [";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ] -";
    if (pass_through) {
        ret += "-";
    } else {
        ret += "X";
    }
    ret += "-> [";
    for (Node *node : waypoints) {
        ret += " " + node->to_string();
    }
    ret += " ] ->";
    return ret;
}

void WaypointPolicy::init(State *state) const
{
    state->comm_state[state->comm].violated = pass_through;
}

void WaypointPolicy::check_violation(State *state)
{
    int fwd_mode = state->comm_state[state->comm].fwd_mode;

    if (fwd_mode == fwd_mode::COLLECT_NHOPS) {
        Node *current_node;
        memcpy(&current_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
        if (waypoints.count(current_node) > 0) {
            // encountering waypoint
            state->comm_state[state->comm].violated = !pass_through;
            state->choice_count = 0;
        }
    }
}
