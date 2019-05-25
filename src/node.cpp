#include "node.hpp"
#include "lib/logger.hpp"

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

        // Add the new interface to intfs and intfs_ipv4
        auto res = intfs.insert(
                       std::pair<std::string, std::shared_ptr<Interface> >
                       (interface->get_name(), interface));
        if (res.second == false) {
            Logger::get_instance().err("Duplicate interface name: " +
                                       res.first->first);
        }
        if (!interface->switching()) {
            auto res2 = intfs_ipv4.insert(
                            std::pair<IPv4Address, std::shared_ptr<Interface> >
                            (interface->addr(), interface));
            if (res2.second == false) {
                Logger::get_instance().err("Duplicate interface IP: " +
                                           res.first->first);
            }
            // TODO add connected route to rib
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

        Route sroute(*net, *nhop);
        auto res = static_routes.insert(sroute);
        if (res.second == false) {
            Logger::get_instance().err("Duplicate static route: " +
                                       res.first->to_string());
        }
    }
}
