#include "network.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "lib/logger.hpp"
#include "middlebox.hpp"
#include "model.h"

void Network::add_node(Node *node)
{
    auto res = nodes.insert(std::make_pair(node->get_name(), node));
    if (res.second == false) {
        Logger::get().err("Duplicate node: " + res.first->first);
    }
}

void Network::add_link(Link *link)
{
    // Add the new link to links
    auto res = links.insert(link);
    if (res.second == false) {
        Logger::get().err("Duplicate link: " + (*res.first)->to_string());
    }

    // Add the new peer to the respective node structures
    Node *node1 = link->get_node1();
    Node *node2 = link->get_node2();
    Interface *intf1 = link->get_intf1();
    Interface *intf2 = link->get_intf2();
    node1->add_peer(intf1->get_name(), node2, intf2);
    node2->add_peer(intf2->get_name(), node1, intf1);
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

void Network::init(State *state __attribute__((unused)))
{
    // initialize and start middlebox emulations
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        node->init();
    }
}

void Network::update_fib(State *state)
{
    EqClass *ec;
    memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));

    FIB *fib = new FIB();
    IPv4Address addr = ec->representative_addr();

    // collect IP next hops
    for (const auto& pair : nodes) {
        Node *node = pair.second;
        fib->set_ipnhs(node, node->get_ipnhs(addr));
    }

    // insert into the pool of history FIBs
    auto res = fibs.insert(fib);
    if (!res.second) {
        delete fib;
        fib = *(res.first);
    }
    memcpy(state->comm_state[state->comm].fib, &fib, sizeof(FIB *));

    Logger::get().debug(fib->to_string());
}

void Network::update_fib_openflow(
    State *state, Node *node, const Route& route,
    const std::vector<Route>& all_updates, size_t num_installed)
{
    // check EC relevance
    EqClass *ec;
    memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));

    if (!route.relevant_to_ec(*ec)) {
        return;
    }

    // check route precedence (longest prefix match)
    bool preferred = true, multipath = false;
    for (size_t i = 0; i < num_installed; ++i) {
        if (all_updates[i] < route) {
            preferred = false;
        } else if (all_updates[i] == route &&
                   !route.has_same_path(all_updates[i])) {
            multipath = true;
        }
    }

    if (!preferred) {
        return;
    }

    // get the next hop
    IPv4Address addr = ec->representative_addr();
    FIB_IPNH next_hop = node->get_ipnh(route.get_intf(), addr);

    if (!next_hop.get_l3_node()) {
        return;
    }

    // construct the new FIB
    FIB *current_fib;
    memcpy(&current_fib, state->comm_state[state->comm].fib, sizeof(FIB *));
    FIB *fib = new FIB(*current_fib);

    if (multipath) {
        fib->add_ipnh(node, std::move(next_hop));
    } else {
        std::set<FIB_IPNH> next_hops;
        next_hops.insert(std::move(next_hop));
        fib->set_ipnhs(node, std::move(next_hops));
    }

    // insert into the pool of history FIBs
    auto res = fibs.insert(fib);
    if (!res.second) {
        delete fib;
        fib = *(res.first);
    }
    memcpy(state->comm_state[state->comm].fib, &fib, sizeof(FIB *));

    Logger::get().debug(fib->to_string());
}
