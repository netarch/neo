#include <memory>
#include <string>

#include "topology.hpp"
#include "node.hpp"
#include "middlebox.hpp"

Topology::Topology(): logger(Logger::get_instance())
{
}

void Topology::load_nodes(
    const std::shared_ptr<cpptoml::table_array>& nodes_config)
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
        }

        if (*type == "generic") {
            node = std::make_shared<Node>(*name);
        } else if (*type == "middlebox") {
            node = std::static_pointer_cast<Node>
                   (std::make_shared<Middlebox>(*name));
            // TODO read "driver", "config", ...
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

        // Add the new node to nodes
        auto res = nodes.insert(std::make_pair(node->get_name(), node));
        if (res.second == false) {
            logger.err("Duplicate node: " + res.first->first);
        }
    }
}

void Topology::load_links(
    const std::shared_ptr<cpptoml::table_array>& links_config)
{
    for (const std::shared_ptr<cpptoml::table>& link_config : *links_config) {
        std::shared_ptr<Link> link;
        auto node1_name = link_config->get_as<std::string>("node1");
        auto intf1_name = link_config->get_as<std::string>("intf1");
        auto node2_name = link_config->get_as<std::string>("node2");
        auto intf2_name = link_config->get_as<std::string>("intf2");

        if (!node1_name) {
            logger.err("Key error: node1");
        }
        if (!intf1_name) {
            logger.err("Key error: intf1");
        }
        if (!node2_name) {
            logger.err("Key error: node2");
        }
        if (!intf2_name) {
            logger.err("Key error: intf2");
        }

        std::shared_ptr<Node>& node1 = nodes[*node1_name];
        std::shared_ptr<Node>& node2 = nodes[*node2_name];
        const std::shared_ptr<Interface>& intf1 =
            node1->get_interface(*intf1_name);
        const std::shared_ptr<Interface>& intf2 =
            node2->get_interface(*intf2_name);

        link = std::make_shared<Link>(node1, intf1, node2, intf2);

        // Add the new link to links
        auto res = links.insert(std::make_pair(*link, link));
        if (res.second == false) {
            logger.err("Duplicate link: " + res.first->second->to_string());
        }

        // Add the new link to the corresponding node structures
        node1->add_peer(*intf1_name, node2, intf2);
        node2->add_peer(*intf2_name, node1, intf1);
        node1->add_link(*intf1_name, link);
        node2->add_link(*intf2_name, link);
    }
}

void Topology::load_config(
    const std::shared_ptr<cpptoml::table_array>& nodes_config,
    const std::shared_ptr<cpptoml::table_array>& links_config)
{
    if (nodes_config) {
        load_nodes(nodes_config);
    }
    logger.info("Loaded " + std::to_string(nodes.size()) + " nodes");

    if (links_config) {
        load_links(links_config);
    }
    logger.info("Loaded " + std::to_string(links.size()) + " links");
}
