#include "policy/policy.hpp"

Policy::Policy(const std::shared_ptr<cpptoml::table>& config)
{
    auto pkt_src_str = config->get_as<std::string>("pkt_src");
    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");

    if (!pkt_src_str) {
        Logger::get_instance().err("Missing packet source");
    }
    if (!pkt_dst_str) {
        Logger::get_instance().err("Missing packet destination");
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
}

void Policy::compute_ecs(const Network& network)
{
    ECs.clear();
    ECs.set_mask_range(pkt_dst);

    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            ECs.add_ec(intf.first);
        }
        for (const Route& route : node.second->get_rib()) {
            ECs.add_ec(route.get_network());
        }
    }
    Logger::get_instance().info("Packet ECs for " + get_type() + ": " +
                                std::to_string(ECs.size()));
}

const IPRange<IPv4Address>& Policy::get_pkt_src() const
{
    return pkt_src;
}

const IPRange<IPv4Address>& Policy::get_pkt_dst() const
{
    return pkt_dst;
}

const EqClasses& Policy::get_ecs() const
{
    return ECs;
}

std::string Policy::to_string() const
{
    return "(policy base class)";
}

std::string Policy::get_type() const
{
    return "(policy base class)";
}

void Policy::procs_init(
    State *state __attribute__((unused)),
    ForwardingProcess& fwd __attribute__((unused))) const
{
}

//bool Policy::check_violation(
//    const Network& net __attribute__((unused)),
//    const ForwardingProcess& fwd __attribute__((unused)))
//{
//    return false;
//}
