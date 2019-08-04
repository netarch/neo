#include <memory>
#include <string>
#include <utility>

#include "network.hpp"
#include "lib/logger.hpp"
#include "middlebox/middlebox.hpp"

Network::Network(): fib(nullptr)
{
}

Network::Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
                 const std::shared_ptr<cpptoml::table_array>& links_config)
    : fib(nullptr)
{
    if (nodes_config) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *nodes_config) {
            Node *node = nullptr;
            auto type = cfg->get_as<std::string>("type");
            if (!type) {
                Logger::get_instance().err("Missing node type");
            }
            if (*type == "generic") {
                node = new Node(cfg);
            } else if (*type == "middlebox") {
                node = new Middlebox(cfg);
            } else {
                Logger::get_instance().err("Unknown node type: " + *type);
            }

            // Add the new node to nodes
            auto res = nodes.insert(std::make_pair(node->get_name(), node));
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
            Link *link = new Link(cfg, nodes);

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

Network::~Network()
{
    for (const auto& node : nodes) {
        delete node.second;
    }
    for (const auto& link : links) {
        delete link;
    }
    for (FIB * const& fib : fibs) {
        delete fib;
    }
    for (FIB_L2DM * const& l2dm : l2dms) {
        delete l2dm;
    }
}

const std::map<std::string, Node *>& Network::get_nodes() const
{
    return nodes;
}

const std::set<Link *, LinkCompare>& Network::get_links() const
{
    return links;
}

void Network::fib_init(const EqClass *ec)
{
    fib = new FIB();
    IPv4Address addr = ec->begin()->get_lb();   // the representative address

    for (const auto& pair : nodes) {
        Node *node = pair.second;

        // collect IP next hops
        fib->set_ipnhs(node, node->get_ipnhs(addr));

        // collect L2 domain
        for (Interface *intf : node->get_intfs_l2()) {
            if (!fib->in_l2dm(intf)) {
                FIB_L2DM *l2_domain = new FIB_L2DM();
                node->collect_l2dm(fib, intf, l2_domain);
                l2dms.insert(l2_domain);
            }
        }
    }

    fibs.insert(fib);
}
