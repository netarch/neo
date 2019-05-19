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
            auto intfs_config = node_config->get_table_array("interfaces");
            node->load_interfaces(intfs_config);
            auto sroutes_config = node_config->get_table_array("static_routes");
            //node->load_static_routes(sroutes_config);
        } else if (*type == "L2" || *type == "L3") {
            node = std::make_shared<Node>(*name, *type);
            auto intfs_config = node_config->get_table_array("interfaces");
            node->load_interfaces(intfs_config);
            auto sroutes_config = node_config->get_table_array("static_routes");
            //node->load_static_routes(sroutes_config);
        } else {
            logger.err("Unknown node type: " + *type);
        }

        add_node(node);
    }

    logger.info("Loaded " + std::to_string(nodes.size()) + " nodes");

    for (const std::shared_ptr<cpptoml::table>& link_config : *links_config) {
        link_config->get_as<std::string>("node1");
        link_config->get_as<std::string>("intf1");
        link_config->get_as<std::string>("node2");
        link_config->get_as<std::string>("intf2");
    }

    //logger.info("Loaded " + std::to_string(links.size()) + " links");
}
