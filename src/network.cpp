#include "network.hpp"

#include "fib.hpp"
#include "middlebox.hpp"
#include "process/openflow.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

void Network::add_node(Node *node)
{
    auto res = nodes.insert(std::make_pair(node->get_name(), node));
    if (res.second == false) {
        Logger::error("Duplicate node: " + res.first->first);
    }
}

void Network::add_link(Link *link)
{
    // Add the new link to links
    auto res = links.insert(link);
    if (res.second == false) {
        Logger::error("Duplicate link: " + (*res.first)->to_string());
    }

    // Add the new peer to the respective node structures
    Node *node1 = link->get_node1();
    Node *node2 = link->get_node2();
    Interface *intf1 = link->get_intf1();
    Interface *intf2 = link->get_intf2();
    node1->add_peer(intf1->get_name(), node2, intf2);
    node2->add_peer(intf2->get_name(), node1, intf1);
}

void Network::add_middlebox(Middlebox *mb)
{
    auto res = middleboxes.insert(mb);
    if (res.second == false) {
        Logger::error("Duplicate middlebox: " + (*res.first)->to_string());
    }
}

void Network::grow_and_set_l2_lan(Node *node, Interface *interface)
{
    L2_LAN *l2_lan = new L2_LAN(node, interface);
    auto res = l2_lans.insert(l2_lan);
    if (!res.second) {
        delete l2_lan;
        l2_lan = *(res.first);
    }
    for (const auto& endpoint : l2_lan->get_l2_endpoints()) {
        endpoint.first->set_l2lan(endpoint.second, l2_lan);
    }
    for (const auto& endpoint : l2_lan->get_l3_endpoints()) {
        endpoint.second.first->set_l2lan(endpoint.second.second, l2_lan);
    }
}

Network(OpenflowProcess *ofp): openflow(ofp)
{
}

Network::~Network()
{
    for (const auto& node : nodes) {
        delete node.second;
    }
    for (const auto& link : links) {
        delete link;
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

const std::unordered_set<Middlebox *>& Network::get_middleboxes() const
{
    return middleboxes;
}

void Network::update_fib(State *state)
{
    FIB fib;
    EqClass *ec = get_ec(state);
    IPv4Address addr = ec->representative_addr();

    // collect IP next hops from routing tables
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        fib.set_ipnhs(node, node->get_ipnhs(addr));
    }

    // install openflow updates that have been installed
    for (auto& pair : openflow->get_installed_updates(state)) {
        fib.set_ipnhs(pair.first, std::move(pair.second));
    }

    FIB *new_fib = set_fib(state, std::move(fib));
    Logger::debug(new_fib->to_string());
}
