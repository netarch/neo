#include <string>
#include <regex>

#include "policy/reachability.hpp"

ReachabilityPolicy::ReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& policy_config,
    const Network& net)
{
    auto pkt_src_str = policy_config->get_as<std::string>("pkt_src");
    auto pkt_dst_str = policy_config->get_as<std::string>("pkt_dst");
    auto start_regex = policy_config->get_as<std::string>("start_node");
    auto final_regex = policy_config->get_as<std::string>("final_node");
    auto reachability = policy_config->get_as<bool>("reachable");

    if (!pkt_src_str) {
        Logger::get_instance().err("Missing packet source");
    }
    if (!pkt_dst_str) {
        Logger::get_instance().err("Missing packet destination");
    }
    if (!start_regex) {
        Logger::get_instance().err("Missing start node");
    }
    if (!final_regex) {
        Logger::get_instance().err("Missing final node");
    }
    if (!reachability) {
        Logger::get_instance().err("Missing reachability");
    }

    std::string src_str = *pkt_src_str;
    std::string dst_str = *pkt_dst_str;
    if (src_str.find('/') == std::string::npos) {
        src_str += "/32";
    }
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    pkt_src = IPRange<IPv4Address>(src_str);
    pkt_dst = IPRange<IPv4Address>(dst_str);

    const std::map<std::string, std::shared_ptr<Node> >& nodes
        = net.get_nodes();
    for (auto node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.push_back(node.second);
        }
    }

    reachable = *reachability;
}

//bool Reachability::check_violation(const Network& net,
//                                   const ForwardingProcess& fwd)
//{
//}
