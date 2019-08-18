#include <regex>
#include <algorithm>

#include "policy/waypoint.hpp"

WaypointPolicy::WaypointPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config)
{
    auto start_regex = config->get_as<std::string>("start_node");
    auto wp_regex = config->get_as<std::string>("waypoint");
    auto through = config->get_as<bool>("pass_through");

    if (!start_regex) {
        Logger::get_instance().err("Missing start node");
    }
    if (!wp_regex) {
        Logger::get_instance().err("Missing waypoint");
    }
    if (!through) {
        Logger::get_instance().err("Missing pass_through");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
        if (std::regex_match(node.first, std::regex(*wp_regex))) {
            waypoints.push_back(node.second);
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

std::string WaypointPolicy::get_type() const
{
    return "waypoint policy";
}

void WaypointPolicy::init()
{
    violated = pass_through;
}

void WaypointPolicy::config_procs(State *state, ForwardingProcess& fwd) const
{
    fwd.config(state, start_nodes);
    fwd.enable();
}

void WaypointPolicy::check_violation(State *state)
{
    auto& current_fwd_mode = state->network_state[state->itr_ec].fwd_mode;

    if (current_fwd_mode == fwd_mode::COLLECT_NHOPS) {
        Node *current_node;
        memcpy(&current_node, state->network_state[state->itr_ec].pkt_location,
               sizeof(Node *));
        if (std::find(waypoints.begin(), waypoints.end(), current_node)
                != waypoints.end()) {
            // encountering waypoint
            violated = !pass_through;
            state->choice_count = 0;
        }
    }
}
