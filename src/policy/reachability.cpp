#include <regex>
#include <algorithm>

#include "policy/reachability.hpp"

ReachabilityPolicy::ReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config)
{
    auto start_regex = config->get_as<std::string>("start_node");
    auto final_regex = config->get_as<std::string>("final_node");
    auto reachability = config->get_as<bool>("reachable");

    if (!start_regex) {
        Logger::get_instance().err("Missing start node");
    }
    if (!final_regex) {
        Logger::get_instance().err("Missing final node");
    }
    if (!reachability) {
        Logger::get_instance().err("Missing reachability");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.insert(node.second);
        }
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

std::string ReachabilityPolicy::get_type() const
{
    return "reachability policy";
}

void ReachabilityPolicy::init(State *state)
{
    state->network_state[state->itr_ec].violated = false;
}

void ReachabilityPolicy::config_procs(State *state,
                                      ForwardingProcess& fwd) const
{
    fwd.config(state, start_nodes);
    fwd.enable();
}

void ReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    auto& current_fwd_mode = state->network_state[state->itr_ec].fwd_mode;

    if (current_fwd_mode == fwd_mode::ACCEPTED) {
        Node *final_node;
        memcpy(&final_node, state->network_state[state->itr_ec].pkt_location,
               sizeof(Node *));
        reached = (final_nodes.count(final_node) > 0);
    } else if (current_fwd_mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        /*
         * If the packet hasn't been accepted or dropped, there is nothing to
         * check.
         */
        return;
    }

    state->network_state[state->itr_ec].violated = (reachable != reached);
    state->choice_count = 0;
}
