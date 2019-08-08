#include <string>

#include "policy/reachability.hpp"

ReachabilityPolicy::ReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config)
{
    auto start_name = config->get_as<std::string>("start_node");
    auto final_name = config->get_as<std::string>("final_node");
    auto reachability = config->get_as<bool>("reachable");

    if (!start_name) {
        Logger::get_instance().err("Missing start node");
    }
    if (!final_name) {
        Logger::get_instance().err("Missing final node");
    }
    if (!reachability) {
        Logger::get_instance().err("Missing reachability");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    start_node = nodes.at(*start_name);
    final_node = nodes.at(*final_name);
    reachable = *reachability;
}

std::string ReachabilityPolicy::to_string() const
{
    std::string ret = "reachability: " + start_node->to_string() + " -";
    if (reachable) {
        ret += "-";
    } else {
        ret += "X";
    }
    ret += "-> " + final_node->to_string();
    return ret;
}

std::string ReachabilityPolicy::get_type() const
{
    return "reachability policy";
}

void ReachabilityPolicy::config_procs(ForwardingProcess& fwd) const
{
    fwd.init(start_node);
    fwd.enable();
}

//bool ReachabilityPolicy::check_violation(
//    const Network& net __attribute__((unused)),
//    const ForwardingProcess& fwd)
//{
//    fwd.
//}
