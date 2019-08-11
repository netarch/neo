#include <string>
#include <regex>

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
            final_nodes.push_back(node.second);
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

void ReachabilityPolicy::config_procs(State *state __attribute__((unused)),
                                      ForwardingProcess& fwd) const
{
    fwd.init();
    fwd.enable();
}

//bool ReachabilityPolicy::check_violation(
//    const Network& net __attribute__((unused)),
//    const ForwardingProcess& fwd)
//{
//    fwd.
//}
