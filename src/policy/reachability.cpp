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

    const std::map<std::string, std::shared_ptr<Node> >& nodes
        = net.get_nodes();
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

//bool ReachabilityPolicy::check_violation(
//    const Network& net __attribute__((unused)),
//    const ForwardingProcess& fwd)
//{
//    fwd.
//}
