#pragma once

#include <map>
#include <set>
#include <string>
#include <unordered_set>

#include "l2-lan.hpp"
#include "link.hpp"
#include "node.hpp"
class Middlebox;
class Route;

/*
 * A network is an undirected graph. Nodes and links will remain constant once
 * they are constructed from the network configurations.
 *
 * The state of a network consists of the current FIB (dataplane) and the failed
 * links.
 */
class Network {
private:
    std::map<std::string, Node *> nodes;
    std::set<Link *, LinkCompare> links;
    std::unordered_set<Middlebox *> middleboxes;

    std::unordered_set<L2_LAN *> l2_lans; // history L2 LANs

private:
    friend class Config;
    void add_node(Node *);
    void add_link(Link *);
    void add_middlebox(Middlebox *);
    void grow_and_set_l2_lan(Node *, Interface *);

public:
    Network() = default;
    Network(const Network &) = delete;
    Network(Network &&) = delete;
    ~Network();

    Network &operator=(const Network &) = delete;
    Network &operator=(Network &&) = delete;

    const std::map<std::string, Node *> &get_nodes() const;
    const std::set<Link *, LinkCompare> &get_links() const;
    const std::unordered_set<Middlebox *> &get_middleboxes() const;
};
