#include "network.hpp"

#include "fib.hpp"
#include "logger.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"

void Network::add_node(Node *node) {
    auto res = nodes.insert(std::make_pair(node->get_name(), node));
    if (res.second == false) {
        logger.error("Duplicate node: " + res.first->first);
    }
}

void Network::add_link(Link *link) {
    // Add the new link to links
    auto res = links.insert(link);
    if (res.second == false) {
        logger.error("Duplicate link: " + (*res.first)->to_string());
    }

    // Add the new peer to the respective node structures
    Node *node1 = link->get_node1();
    Node *node2 = link->get_node2();
    Interface *intf1 = link->get_intf1();
    Interface *intf2 = link->get_intf2();
    node1->add_peer(intf1->get_name(), node2, intf2);
    node2->add_peer(intf2->get_name(), node1, intf1);
}

void Network::add_middlebox(Middlebox *mb) {
    auto res = middleboxes.insert(mb);
    if (res.second == false) {
        logger.error("Duplicate middlebox: " + (*res.first)->to_string());
    }
}

void Network::grow_and_set_l2_lan(Node *node, Interface *interface) {
    L2_LAN *l2_lan = new L2_LAN(node, interface);
    auto res = l2_lans.insert(l2_lan);
    if (!res.second) {
        delete l2_lan;
        l2_lan = *(res.first);
    }
    for (const auto &endpoint : l2_lan->get_l2_endpoints()) {
        endpoint.first->set_l2lan(endpoint.second, l2_lan);
    }
    for (const auto &endpoint : l2_lan->get_l3_endpoints()) {
        endpoint.second.first->set_l2lan(endpoint.second.second, l2_lan);
    }
}

Network::~Network() {
    this->reset();
}

void Network::reset() {
    for (const auto &[name, node] : this->nodes) {
        delete node;
    }

    this->nodes.clear();

    for (const auto &link : this->links) {
        delete link;
    }

    this->links.clear();

    for (L2_LAN *const &l2_lan : this->l2_lans) {
        delete l2_lan;
    }

    this->l2_lans.clear();
}

const std::map<std::string, Node *> &Network::get_nodes() const {
    return nodes;
}

const std::set<Link *, LinkCompare> &Network::get_links() const {
    return links;
}

const std::unordered_set<Middlebox *> &Network::get_middleboxes() const {
    return middleboxes;
}
