#include "network.hpp"

#include <memory>
#include <string>
#include <utility>
#include <cstring>

#include "middlebox.hpp"
#include "lib/logger.hpp"

Network::Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
                 const std::shared_ptr<cpptoml::table_array>& links_config)
{
    if (nodes_config) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *nodes_config) {
            Node *node = nullptr;
            auto type = cfg->get_as<std::string>("type");
            if (!type) {
                Logger::get().err("Missing node type");
            }
            if (*type == "generic") {
                node = new Node(cfg);
            } else if (*type == "middlebox") {
                node = new Middlebox(cfg);
            } else {
                Logger::get().err("Unknown node type: " + *type);
            }

            // Add the new node to nodes
            auto res = nodes.insert(std::make_pair(node->get_name(), node));
            if (res.second == false) {
                Logger::get().err("Duplicate node: " + res.first->first);
            }
        }
    }
    Logger::get().info("Loaded " + std::to_string(nodes.size()) + " nodes");
    if (links_config) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *links_config) {
            Link *link = new Link(cfg, nodes);

            // Add the new link to links
            auto res = links.insert(link);
            if (res.second == false) {
                Logger::get().err("Duplicate link: " +
                                  (*res.first)->to_string());
            }

            // Add the new peer to the respective node structures
            auto node1 = link->get_node1();
            auto node2 = link->get_node2();
            auto intf1 = link->get_intf1();
            auto intf2 = link->get_intf2();
            node1->add_peer(intf1->get_name(), node2, intf2);
            node2->add_peer(intf2->get_name(), node1, intf1);
        }
    }
    Logger::get().info("Loaded " + std::to_string(links.size()) + " links");

    // collect L2 LANs
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        for (Interface *intf : node->get_intfs_l2()) {
            if (!node->mapped_to_l2lan(intf)) {
                L2_LAN *l2_lan = new L2_LAN(node, intf);
                auto res = l2_lans.insert(l2_lan);
                if (!res.second) {
                    delete l2_lan;
                    l2_lan = *(res.first);
                }
                for (const auto& ep : l2_lan->get_l2_endpoints()) {
                    ep.first->set_l2lan(ep.second, l2_lan);
                }
            }
        }
    }
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
    for (L2_LAN * const& l2_lan : l2_lans) {
        delete l2_lan;
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

void Network::init(State *state)
{
    // initialize FIB
    EqClass *cur_ec;
    memcpy(&cur_ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
    fib_init(state, cur_ec);

    // initialize and start middlebox emulations
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        node->init();
    }

    // TODO: initialize update history if update agent is implemented
}

void Network::fib_init(State *state, const EqClass *ec)
{
    FIB *fib = new FIB();
    IPv4Address addr = ec->begin()->get_lb();   // the representative address

    // collect IP next hops
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        fib->set_ipnhs(node, node->get_ipnhs(addr));
    }

    auto res = fibs.insert(fib);
    if (!res.second) {
        delete fib;
        fib = *(res.first);
    }
    memcpy(state->comm_state[state->comm].fib, &fib, sizeof(FIB *));

    Logger::get().info(fib->to_string());
}
