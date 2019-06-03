#include <utility>

#include "node.hpp"
#include "lib/logger.hpp"

void Node::rib_install(const Route& route)
{
    auto res = rib.insert(route);
    if (res.second == false) {
        if (res.first->get_adm_dist() < route.get_adm_dist()) {
            // with lower priority
            Logger::get_instance().warn("Route ignored: " + route.to_string());
        } else if (res.first->get_adm_dist() > route.get_adm_dist()) {
            // with higher priority
            auto it = rib.erase(res.first);
            rib.insert(it, route);
        } else {
            // having the same administrative distance
            if (!res.first->identical(route)) {
                Logger::get_instance().err("RIB entry mismatch: " +
                                           res.first->to_string());
            }
        }
    }
}

Node::Node(const std::string& name, const std::string& type)
    : name(name), type(type)
{
    if (type != "generic") {
        Logger::get_instance().err("Unsupported node type: " + type);
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

std::string Node::get_type() const
{
    return type;
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

void
Node::load_interfaces(const std::shared_ptr<cpptoml::table_array>& config)
{
    for (const std::shared_ptr<cpptoml::table>& intf_config : *config) {
        std::shared_ptr<Interface> interface;
        auto name = intf_config->get_as<std::string>("name");
        auto ipv4 = intf_config->get_as<std::string>("ipv4");

        if (!name) {
            Logger::get_instance().err("Key error: name");
        }
        if (ipv4) {
            interface = std::make_shared<Interface>(*name, *ipv4);
        } else {
            interface = std::make_shared<Interface>(*name);
        }

        // Add the new interface to intfs
        auto res = intfs.insert
                   (std::make_pair(interface->get_name(), interface));
        if (res.second == false) {
            Logger::get_instance().err("Duplicate interface name: " +
                                       res.first->first);
        }
        // Add the new interface to intfs_ipv4
        if (!interface->switching()) {
            auto res = intfs_ipv4.insert
                       (std::make_pair(interface->addr(), interface));
            if (res.second == false) {
                Logger::get_instance().err("Duplicate interface IP: " +
                                           res.first->first.to_string());
            }

            // Add the directly connected route to rib
            rib_install(Route(interface->network(), interface->addr(),
                              interface->get_name(), 0));
        }
    }
}

void
Node::load_static_routes(const std::shared_ptr<cpptoml::table_array>& config)
{
    for (const std::shared_ptr<cpptoml::table>& sroute_config : *config) {
        auto net = sroute_config->get_as<std::string>("network");
        auto nhop = sroute_config->get_as<std::string>("next_hop");

        if (!net) {
            Logger::get_instance().err("Key error: network");
        }
        if (!nhop) {
            Logger::get_instance().err("Key error: next_hop");
        }

        Route sroute(*net, *nhop, 1);

        // Add the new static route to static_routes
        auto res = static_routes.insert(sroute);
        if (res.second == false) {
            Logger::get_instance().err("Duplicate static route: " +
                                       res.first->to_string());
        }

        // Add the static route to rib
        rib_install(sroute);
    }
}

void
Node::load_installed_routes(const std::shared_ptr<cpptoml::table_array>& config)
{
    for (const std::shared_ptr<cpptoml::table>& iroute_config : *config) {
        auto net = iroute_config->get_as<std::string>("network");
        auto nhop = iroute_config->get_as<std::string>("next_hop");

        if (!net) {
            Logger::get_instance().err("Key error: network");
        }
        if (!nhop) {
            Logger::get_instance().err("Key error: next_hop");
        }

        rib_install(Route(*net, *nhop));
    }
}
