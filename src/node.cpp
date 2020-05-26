#include "node.hpp"

#include <utility>
#include <stdexcept>

#include "lib/logger.hpp"

Node::Node(const std::shared_ptr<cpptoml::table>& config)
{
    auto node_name = config->get_as<std::string>("name");
    auto intfs_cfg = config->get_table_array("interfaces");
    auto srs_cfg = config->get_table_array("static_routes");
    auto irs_cfg = config->get_table_array("installed_routes");

    if (!node_name) {
        Logger::get().err("Missing node name");
    }
    name = std::move(*node_name);

    if (intfs_cfg) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *intfs_cfg) {
            Interface *intf = new Interface(cfg);
            // Add the new interface to intfs
            auto res = intfs.insert(std::make_pair(intf->get_name(), intf));
            if (res.second == false) {
                Logger::get().err("Duplicate interface name: " +
                                  res.first->first);
            }
            if (intf->is_l2()) {
                // Add the new interface to intfs_l2
                intfs_l2.insert(intf);
            } else {
                // Add the new interface to intfs_l3
                auto res = intfs_l3.insert(std::make_pair(intf->addr(), intf));
                if (res.second == false) {
                    Logger::get().err("Duplicate interface IP: " +
                                      res.first->first.to_string());
                }

                // Add the directly connected route to rib
                rib.emplace(intf->network(), intf->addr(), intf->get_name(), 0);
            }
        }
    }
    if (srs_cfg) {
        for (const std::shared_ptr<cpptoml::table>& sr_cfg : *srs_cfg) {
            Route route(sr_cfg);
            if (route.get_adm_dist() == 255) {  // user did not specify adm dist
                route.set_adm_dist(1);
            }
            rib.insert(std::move(route));
        }
    }
    if (irs_cfg) {
        for (const std::shared_ptr<cpptoml::table>& ir_cfg : *irs_cfg) {
            rib.emplace(ir_cfg);
        }
    }
}

Node::~Node()
{
    for (const auto& intf : intfs) {
        delete intf.second;
    }
}

void Node::init()
{
}

std::string Node::to_string() const
{
    return name;
}

std::string Node::get_name() const
{
    return name;
}

bool Node::has_ip(const IPv4Address& addr) const
{
    return intfs_l3.count(addr) > 0;
}

bool Node::is_l3_only() const
{
    return intfs_l2.empty();
}

Interface *Node::get_interface(const std::string& intf_name) const
{
    auto intf = intfs.find(intf_name);
    if (intf == intfs.end()) {
        Logger::get().err(to_string() + " doesn't have interface " + intf_name);
    }
    return intf->second;
}

Interface *Node::get_interface(const char *intf_name) const
{
    return get_interface(std::string(intf_name));
}

Interface *Node::get_interface(const IPv4Address& addr) const
{
    auto intf = intfs_l3.find(addr);
    if (intf == intfs_l3.end()) {
        Logger::get().err(to_string() + " doesn't own " + addr.to_string());
    }
    return intf->second;
}

const std::map<std::string, Interface *>& Node::get_intfs() const
{
    return intfs;
}

const std::map<IPv4Address, Interface *>& Node::get_intfs_l3() const
{
    return intfs_l3;
}

const std::set<Interface *>& Node::get_intfs_l2() const
{
    return intfs_l2;
}

const RoutingTable& Node::get_rib() const
{
    return rib;
}

RoutingTable& Node::get_rib()
{
    return rib;
}

std::pair<Node *, Interface *>
Node::get_peer(const std::string& intf_name) const
{
    auto peer = l2_peers.find(intf_name);
    if (peer == l2_peers.end()) {
        return std::make_pair(nullptr, nullptr);
    }
    return peer->second;
}

void Node::add_peer(const std::string& intf_name, Node *node,
                    Interface *intf)
{
    auto res = l2_peers.insert
               (std::make_pair(intf_name, std::make_pair(node, intf)));
    if (res.second == false) {
        Logger::get().err("Two peers on interface: " + intf_name);
    }
}

bool Node::mapped_to_l2lan(Interface *intf) const
{
    return l2_lans.count(intf) > 0;
}

void Node::set_l2lan(Interface *intf, L2_LAN *l2_lan)
{
    l2_lans[intf] = l2_lan;
}

L2_LAN *Node::get_l2lan(Interface *intf) const
{
    return l2_lans.at(intf);
}

std::set<FIB_IPNH> Node::get_ipnhs(const IPv4Address& dst)
{
    std::set<FIB_IPNH> next_hops;

    auto r = rib.lookup(dst);
    for (RoutingTable::const_iterator it = r.first; it != r.second; ++it) {
        if (it->get_intf().empty()) {
            // non-connected route; look it up recursively
            auto nhs = get_ipnhs(it->get_next_hop());
            next_hops.insert(nhs.begin(), nhs.end());
        } else if (has_ip(dst)) {
            // connected route; accept
            next_hops.insert(FIB_IPNH(this, nullptr, this, nullptr));
            //                        l3nh l3nh_intf l2nh l2nh_intf
        } else {
            // connected route; forward
            auto l2nh = get_peer(it->get_intf());   // L2 next hop
            if (l2nh.first) { // if the interface is truly connected (has peer)
                if (!l2nh.second->is_l2()) {    // L2 next hop == L3 next hop
                    next_hops.insert(FIB_IPNH(l2nh.first, l2nh.second,
                                              l2nh.first, l2nh.second));
                } else {
                    auto l3nh =
                        l2nh.first->l2_lans[l2nh.second]->find_l3_endpoint(dst);
                    if (l3nh.first) {
                        next_hops.insert(FIB_IPNH(l3nh.first, l3nh.second,
                                                  l2nh.first, l2nh.second));
                    }
                }
            }
        }
    }

    return next_hops;
}
