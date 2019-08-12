#pragma once

#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "link.hpp"
#include "fib.hpp"
#include "eqclass.hpp"
#include "pan.h"

/*
 * A network is an undirected graph. Nodes and links will remain constant once
 * they are constructed from the network configurations.
 *
 * The state of a network consists of the current FIB (dataplane) and the failed
 * links.
 *
 * "fibs" records all the history FIBs to prevent duplicate.
 */
class Network
{
private:
    std::map<std::string, Node *>   nodes;
    std::set<Link *, LinkCompare>   links;

    FIB *fib;   // the current FIB for this EC
    std::unordered_set<FIB *> fibs;         // history FIBs
    std::unordered_set<FIB_L2DM *> l2dms;   // history L2 domains

public:
    Network();
    Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
            const std::shared_ptr<cpptoml::table_array>& links_config);
    Network(const Network&) = delete;
    Network(Network&&) = default;
    ~Network();

    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = default;

    const std::map<std::string, Node *>& get_nodes() const;
    const std::set<Link *, LinkCompare>& get_links() const;

    void fib_init(State *, const EqClass *);   // compute the initial FIB
    const std::set<FIB_IPNH>& fib_lookup(Node *const) const;
    FIB_L2DM *const& fib_lookup(Interface *const) const;

    // The failure agent/process is not implemented yet, but if a link fails,
    // the FIB would need to be updated. (A link failure will change the
    // "active_peers" of the related nodes, and hence potentially change the
    // FIB.)
};
