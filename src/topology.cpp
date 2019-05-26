#include <memory>
#include <string>

#include "topology.hpp"
#include "node.hpp"
#include "middlebox.hpp"

Topology::Topology(): logger(Logger::get_instance())
{
}

void Topology::add_node(const std::shared_ptr<Node>& node)
{
    auto res = nodes.insert(std::pair<std::string, std::shared_ptr<Node> >
                            (node->get_name(), node));
    if (res.second == false) {
        logger.err("Duplicate node: " + res.first->first);
    }
}

void Topology::load_nodes(
    const std::shared_ptr<cpptoml::table_array> nodes_config)
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
            // TODO read "driver", "config", ...
        } else if (*type == "L2" || *type == "L3") {
            node = std::make_shared<Node>(*name, *type);
        } else {
            logger.err("Unknown node type: " + *type);
        }

        if (auto config = node_config->get_table_array("interfaces")) {
            node->load_interfaces(config);
        }
        if (auto config = node_config->get_table_array("static_routes")) {
            node->load_static_routes(config);
        }
        if (auto config = node_config->get_table_array("installed_routes")) {
            node->load_installed_routes(config);
        }

        add_node(node);
    }
}

void Topology::load_links(
    const std::shared_ptr<cpptoml::table_array> links_config)
{
    for (const std::shared_ptr<cpptoml::table>& link_config : *links_config) {
        link_config->get_as<std::string>("node1");
        link_config->get_as<std::string>("intf1");
        link_config->get_as<std::string>("node2");
        link_config->get_as<std::string>("intf2");
    }
}

void Topology::load_config(
    const std::shared_ptr<cpptoml::table_array> nodes_config,
    const std::shared_ptr<cpptoml::table_array> links_config)
{
    if (nodes_config) {
        load_nodes(nodes_config);
    }
    logger.info("Loaded " + std::to_string(nodes.size()) + " nodes");

    if (links_config) {
        load_links(links_config);
    }
    //logger.info("Loaded " + std::to_string(links.size()) + " links");
}
