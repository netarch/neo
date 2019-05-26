#include "node.hpp"
#include "lib/logger.hpp"

void Node::rib_install(const Route& route)
{
    auto res = rib.insert(route);
    if (res.second == false) {
        if (res.first->get_adm_dist() < route.get_adm_dist()) {
            Logger::get_instance().warn("Route ignored: " + route.to_string());
            return;
        } else if (res.first->get_adm_dist() > route.get_adm_dist()) {
            auto it = rib.erase(res.first);
            res.first = rib.insert(it, route);
        }
        if (!res.first->identical(route)) {
            Logger::get_instance().err("RIB entry mismatch: " +
                                       res.first->to_string());
        }
    }
}

Node::Node(const std::string& name, const std::string& type)
    : name(name), type(type)
{
}

std::string Node::get_name() const
{
    return name;
}

void Node::load_interfaces(
    const std::shared_ptr<cpptoml::table_array> config)
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
        auto res = intfs.insert(
                       std::pair<std::string, std::shared_ptr<Interface> >
                       (interface->get_name(), interface));
        if (res.second == false) {
            Logger::get_instance().err("Duplicate interface name: " +
                                       res.first->first);
        }
        // Add the new interface to intfs_ipv4
        if (!interface->switching()) {
            auto res = intfs_ipv4.insert(
                           std::pair<IPv4Address, std::shared_ptr<Interface> >
                           (interface->addr(), interface));
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

void Node::load_static_routes(
    const std::shared_ptr<cpptoml::table_array> config)
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

void Node::load_installed_routes(
    const std::shared_ptr<cpptoml::table_array> config)
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
