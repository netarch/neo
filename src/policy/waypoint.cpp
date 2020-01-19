#include "policy/waypoint.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

WaypointPolicy::WaypointPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy()
{
    parse_protocol(config);
    parse_pkt_dst(config);
    parse_owned_dst_only(config);
    parse_start_node(config, net);
    parse_tcp_ports(config);
    parse_waypoint(config, net);
    parse_pass_through(config);
}

void WaypointPolicy::parse_waypoint(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
{
    auto wp_regex = config->get_as<std::string>("waypoint");

    if (!wp_regex) {
        Logger::get().err("Missing waypoint");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*wp_regex))) {
            waypoints.insert(node.second);
        }
    }
}

void WaypointPolicy::parse_pass_through(
    const std::shared_ptr<cpptoml::table>& config)
{
    auto through = config->get_as<bool>("pass_through");

    if (!through) {
        Logger::get().err("Missing pass_through");
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
    state->violated = pass_through;
}

void WaypointPolicy::check_violation(State *state)
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
}
