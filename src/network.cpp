#include "network.hpp"

#include "logger.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"

void Network::add_node(Node *node) {
    auto res = _nodes.insert(std::make_pair(node->get_name(), node));
    if (res.second == false) {
        logger.error("Duplicate node: " + res.first->first);
    }
}

void Network::add_link(Link *link) {
    // Add the new link to _links
    auto res = _links.insert(link);
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
    auto res = _mbs.insert(mb);
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
    for (const auto &[node, intf] : l2_lan->get_l2_endpoints()) {
        node->set_l2lan(intf, l2_lan);
    }
    for (const auto &[addr, ep] : l2_lan->get_l3_endpoints()) {
        ep.first->set_l2lan(ep.second, l2_lan);
    }
}

Network::~Network() {
    this->reset();
}

void Network::reset() {
    for (const auto &[name, node] : this->_nodes) {
        delete node;
    }

    this->_nodes.clear();
    this->_mbs.clear();

    for (const auto &link : this->_links) {
        delete link;
    }

    this->_links.clear();

    for (L2_LAN *const &l2_lan : this->l2_lans) {
        delete l2_lan;
    }

    this->l2_lans.clear();
}
