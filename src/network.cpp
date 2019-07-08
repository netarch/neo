#include <memory>
#include <string>
#include <utility>

#include "network.hpp"
#include "lib/logger.hpp"
#include "middlebox/middlebox.hpp"

Network::Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
                 const std::shared_ptr<cpptoml::table_array>& links_config)
{
    if (nodes_config) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *nodes_config) {
            std::shared_ptr<Node> node;
            auto type = cfg->get_as<std::string>("type");
            if (!type) {
                Logger::get_instance().err("Missing node type");
            }
            if (*type == "generic") {
                node = std::make_shared<Node>(cfg);
            } else if (*type == "middlebox") {
                node = std::static_pointer_cast<Node>
                       (std::make_shared<Middlebox>(cfg));
            } else {
                Logger::get_instance().err("Unknown node type: " + *type);
            }

            // Add the new node to nodes
            auto res = nodes.insert(std::make_pair(node->get_name(),
                                                   std::move(node)));
            if (res.second == false) {
                Logger::get_instance().err("Duplicate node: " +
                                           res.first->first);
            }
        }
    }
    Logger::get_instance().info("Loaded " + std::to_string(nodes.size()) +
                                " nodes");
    if (links_config) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *links_config) {
            std::shared_ptr<Link> link = std::make_shared<Link>(cfg, nodes);

            // Add the new link to links
            auto res = links.insert(link);
            if (res.second == false) {
                Logger::get_instance().err("Duplicate link: " +
                                           (*res.first)->to_string());
            }

            // Add the new link to the corresponding node structures
            auto node1 = link->get_node1();
            auto node2 = link->get_node2();
            auto intf1 = link->get_intf1();
            auto intf2 = link->get_intf2();
            node1->add_peer(intf1->get_name(), node2, intf2);
            node2->add_peer(intf2->get_name(), node1, intf1);
            node1->add_link(intf1->get_name(), link);
            node2->add_link(intf2->get_name(), link);
        }
    }
    Logger::get_instance().info("Loaded " + std::to_string(links.size()) +
                                " links");
}

const std::map<std::string, std::shared_ptr<Node> >& Network::get_nodes() const
{
    return nodes;
}

const std::set<std::shared_ptr<Link>, LinkCompare>& Network::get_links() const
{
    return links;
}
