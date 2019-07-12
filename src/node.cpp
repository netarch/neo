#include <utility>

#include "node.hpp"
#include "lib/logger.hpp"

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
            std::shared_ptr<Interface> intf = std::make_shared<Interface>(cfg);
            // Add the new interface to intfs
            auto res = intfs.insert
                       (std::make_pair(intf->get_name(), intf));
            if (res.second == false) {
                Logger::get_instance().err("Duplicate interface name: " +
                                           res.first->first);
            }
            if (!intf->switching()) {
                // Add the new interface to intfs_ipv4
                auto res = intfs_ipv4.insert
                           (std::make_pair(intf->addr(), intf));
                if (res.second == false) {
                    Logger::get_instance().err("Duplicate interface IP: " +
                                               res.first->first.to_string());
                }

                // Add the directly connected route to rib
                rib.emplace(intf->network(), intf->addr(), 0, intf->get_name());
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
    return intfs_ipv4.count(addr) > 0;
}

bool Node::has_ip(const std::string& addr) const
{
    return has_ip(IPv4Address(addr));
}

const std::shared_ptr<Interface>&
Node::get_interface(const std::string& intf_name) const
{
    return intfs.at(intf_name);
}

const std::shared_ptr<Interface>&
Node::get_interface(const IPv4Address& addr) const
{
    return intfs_ipv4.at(addr);
}

const std::map<IPv4Address, std::shared_ptr<Interface> >&
Node::get_intfs_ipv4() const
{
    return intfs_ipv4;
}

const RoutingTable& Node::get_rib() const
{
    return rib;
}

std::set<std::shared_ptr<Node> >
Node::get_next_hops(const std::shared_ptr<Node>& self,
                    const IPv4Address& dst) const
{
    std::set<std::shared_ptr<Node> > next_hops;

    auto r = rib.lookup(dst);
    for (RoutingTable::const_iterator it = r.first; it != r.second; ++it) {
        if (it->get_ifname().empty()) {
            // non-connected routes; look it up recursively
            auto nhs = get_next_hops(self, it->get_next_hop());
            next_hops.insert(nhs.begin(), nhs.end());
        } else {
            // connected routes; accept it or forward it
            if (intfs_ipv4.find(dst) != intfs_ipv4.end()) {
                next_hops.insert(self);
            } else {
                auto peer = active_peers.find(it->get_ifname());
                if (peer != active_peers.end()) {
                    next_hops.insert(peer->second.first.lock());
                }
            }
        }
    }

    return next_hops;
}

std::pair<std::shared_ptr<Node>, std::shared_ptr<Interface> >
Node::get_peer(const std::string& intf_name) const
{
    auto weakpair = active_peers.at(intf_name);
    return std::make_pair(weakpair.first.lock(), weakpair.second.lock());
}

std::shared_ptr<Link>
Node::get_link(const std::string& intf_name) const
{
    return active_links.at(intf_name).lock();
}

void Node::add_peer(const std::string& intf_name,
                    const std::shared_ptr<Node>& node,
                    const std::shared_ptr<Interface>& intf)
{
    auto res = active_peers.insert
               (std::make_pair
                (intf_name, std::make_pair
                 (std::weak_ptr<Node>(node), std::weak_ptr<Interface>(intf))));
    if (res.second == false) {
        Logger::get_instance().err("Two peers on interface: " + intf_name);
    }
}

void Node::add_link(const std::string& intf_name,
                    const std::shared_ptr<Link>& link)
{
    auto res = active_links.insert
               (std::make_pair(intf_name, std::weak_ptr<Link>(link)));
    if (res.second == false) {
        Logger::get_instance().err("Two links on interface: " + intf_name);
    }
}
