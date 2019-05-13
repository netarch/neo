#include <memory>

#include "topology.hpp"
#include "node.hpp"
#include "middlebox.hpp"

Topology::Topology(): logger(Logger::get_instance())
{
}

void Topology::load_config(
    const std::shared_ptr<cpptoml::table_array> nodes_config,
    const std::shared_ptr<cpptoml::table_array> links_config)
{
    for (const std::shared_ptr<cpptoml::table>& node_config : *nodes_config) {
        std::shared_ptr<Node> node;
        auto name = node_config->get_as<std::string>("name");
        auto type = node_config->get_as<std::string>("type");

        if (!name) {
            logger.err("Key error: name");
        }
        if (!type) {
            logger.err("Key error: type");
        } else if (*type == "middlebox") {
            node = std::static_pointer_cast<Node>
                   (std::make_shared<Middlebox>(*name, *type));
        } else if (*type == "L2" || *type == "L3") {
            node = std::make_shared<Node>(*name, *type);
        } else {
            logger.err("Unknown node type: " + *type);
        }

        auto res = nodes.insert(std::pair<std::string,
                                std::shared_ptr<Node> >(*name, node));
        if (res.second == false) {
            logger.err("Duplicate node: " + res.first->first);
        }
    }

    for (const std::shared_ptr<cpptoml::table>& link_config : *links_config) {
        link_config->get_as<std::string>("node1");
        link_config->get_as<std::string>("intf1");
        link_config->get_as<std::string>("node2");
        link_config->get_as<std::string>("intf2");
    }
}
