#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "l2-lan.hpp"
#include "link.hpp"
#include "node.hpp"

class Middlebox;
class Plankton;
class Route;

/**
 * A network is an undirected graph. Nodes and links will remain constant once
 * they are constructed from the input configuration.
 *
 * The state of a network consists of the current FIB (dataplane) and the failed
 * links.
 */
class Network {
private:
    Plankton *_plankton;
    std::unordered_map<std::string, Node *> _nodes;
    std::set<Link *, LinkCompare> _links;
    std::unordered_set<Middlebox *> _mbs;

    std::unordered_set<L2_LAN *> l2_lans; // history L2 LANs

private:
    friend class ConfigParser;
    void add_node(Node *);
    void add_link(Link *);
    void add_middlebox(Middlebox *);
    void grow_and_set_l2_lan(Node *, Interface *);

public:
    Network()                = default;
    Network(const Network &) = delete;
    Network(Network &&)      = delete;
    ~Network();

    Network &operator=(const Network &) = delete;
    Network &operator=(Network &&)      = delete;

    void reset();
    const decltype(_nodes) &nodes() const { return _nodes; }
    const decltype(_links) &links() const { return _links; }
    const decltype(_mbs) &middleboxes() const { return _mbs; }
};
