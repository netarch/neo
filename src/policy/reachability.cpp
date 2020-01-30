#include "policy/reachability.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

ReachabilityPolicy::ReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    parse_protocol(config);
    parse_pkt_dst(config);
    parse_owned_dst_only(config);
    parse_start_node(config, net);
    parse_tcp_ports(config);
    parse_final_node(config, net);
    parse_reachable(config);
}

void ReachabilityPolicy::parse_final_node(
    const std::shared_ptr<cpptoml::table>& config, const Network& net)
{
    auto final_regex = config->get_as<std::string>("final_node");

    if (!final_regex) {
        Logger::get().err("Missing final node");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.insert(node.second);
        }
    }
}

void ReachabilityPolicy::parse_reachable(
    const std::shared_ptr<cpptoml::table>& config)
{
    auto reachability = config->get_as<bool>("reachable");

    if (!reachability) {
        Logger::get().err("Missing reachability");
    }

    reachable = *reachability;
}

std::string ReachabilityPolicy::to_string() const
{
    std::string ret = "reachability [";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ] -";
    if (reachable) {
        ret += "-";
    } else {
        ret += "X";
    }
    ret += "-> [";
    for (Node *node : final_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void ReachabilityPolicy::init(State *state) const
{
    state->violated = false;
}

void ReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (mode == fwd_mode::ACCEPTED &&
            (pkt_state == PS_HTTP_REQ || pkt_state == PS_ICMP_ECHO_REQ)) {
        reached = (final_nodes.count(comm_rx) > 0);
    } else if (mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        /*
         * If the packet hasn't been accepted or dropped, there is nothing to
         * check.
         */
        return;
    }

    state->violated = (reachable != reached);
    state->choice_count = 0;
}
