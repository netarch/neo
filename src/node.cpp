#include "node.hpp"
#include "logger.hpp"

Node::Node(const std::string& name, const std::string& type)
    : name(name), type(type)
{
}

const std::string& Node::get_name() const
{
    return name;
}

void Node::load_interfaces(
    const std::shared_ptr<cpptoml::table_array> intfs_config)
{
    for (const std::shared_ptr<cpptoml::table>& intf_config : *intfs_config) {
        std::shared_ptr<Interface> interface;
        auto name = intf_config->get_as<std::string>("name");
        auto ipv4 = intf_config->get_as<std::string>("ipv4");

        if (!name) {
            Logger::get_instance().err("Key error: name");
        }
        interface = std::make_shared<Interface>(*name);
        if (ipv4) {
            //interface->set_ipv4(*ipv4);
        }
    }
}
