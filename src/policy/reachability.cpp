#include <string>

#include "policy/reachability.hpp"

void ReachabilityPolicy::load_config(
    const std::shared_ptr<cpptoml::table>& policy_config,
    const Network& net)
{
    auto pkt_src_str = policy_config->get_as<std::string>("pkt_src");
    auto pkt_dst_str = policy_config->get_as<std::string>("pkt_dst");
    auto start_regex = policy_config->get_as<std::string>("start_node");
    auto final_regex = policy_config->get_as<std::string>("final_node");
    auto reachability = policy_config->get_as<bool>("reachable");

    if (!pkt_src_str) {
        Logger::get_instance().err("Key error: pkt_src");
    }
    if (!pkt_dst_str) {
        Logger::get_instance().err("Key error: pkt_dst");
    }
    if (!start_regex) {
        Logger::get_instance().err("Key error: start_node");
    }
    if (!final_regex) {
        Logger::get_instance().err("Key error: final_node");
    }
    if (!reachability) {
        Logger::get_instance().err("Key error: reachable");
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
        // TODO
    }
}

//bool Reachability::check_violation(const Network& net,
//                                   const ForwardingProcess& fwd)
//{
//}
