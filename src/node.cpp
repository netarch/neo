#include <utility>
#include <stdexcept>

#include "node.hpp"
#include "lib/logger.hpp"

static IPList iplist;

Node::Node(const std::shared_ptr<cpptoml::table>& config)
{
    auto node_name = config->get_as<std::string>("name");
    auto intfs_cfg = config->get_table_array("interfaces");
    auto srs_cfg = config->get_table_array("static_routes");
    auto irs_cfg = config->get_table_array("installed_routes");

    if (!node_name) {
        Logger::get_instance().err("Missing node name");
    }
    name = std::move(*node_name);

    if (intfs_cfg) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *intfs_cfg) {
            Interface *intf = new Interface(cfg);
            // Add the new interface to intfs
            auto res = intfs.insert(std::make_pair(intf->get_name(), intf));
            if (res.second == false) {
                Logger::get_instance().err("Duplicate interface name: " +
                                           res.first->first);
            }
            if (intf->switching()) {
                // Add the new interface to intfs_l2
                intfs_l2.insert(intf);
            } else {
                // Add the new interface to intfs_l3
                auto res = intfs_l3.insert(std::make_pair(intf->addr(), intf));
                if (res.second == false) {
                    Logger::get_instance().err("Duplicate interface IP: " +
                                               res.first->first.to_string());
                }

                // Add the directly connected route to rib
                rib.emplace(intf->network(), intf->addr(), intf->get_name(), 0);

                // Add to the IP list
                iplist.add(intf->addr(), this);
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
        Logger::get_instance().err(to_string() + " doesn't have interface "
                                   + intf_name);
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
        Logger::get_instance().err(to_string() + " doesn't own "
                                   + addr.to_string());
    }
    return intf->second;
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
            next_hops.insert(FIB_IPNH(this, this, nullptr));
        } else {
            // connected route; forward
            auto peer = get_peer(it->get_intf());   // L2 next hop
            Node *dst_node = iplist.get_node(dst);  // L3 next hop
            if (peer.first && dst_node) {
                next_hops.insert(FIB_IPNH(dst_node, peer.first, peer.second));
            }
        }
    }

    return next_hops;
}

std::pair<Node *, Interface *>
Node::get_peer(const std::string& intf_name) const
{
    auto peer = active_peers.find(intf_name);
    if (peer == active_peers.end()) {
        return std::make_pair(nullptr, nullptr);
    }
    return peer->second;
}

void Node::add_peer(const std::string& intf_name, Node *node, Interface *intf)
{
    auto res = active_peers.insert
               (std::make_pair(intf_name, std::make_pair(node, intf)));
    if (res.second == false) {
        Logger::get_instance().err("Two peers on interface: " + intf_name);
    }
}

void IPList::add(const IPv4Address& addr, Node *const node)
{
    tbl[addr] = node;
}

Node *IPList::get_node(const IPv4Address& addr) const
{
    auto node = tbl.find(addr);
    if (node == tbl.end()) {
        return nullptr;
    }
    return node->second;
}
