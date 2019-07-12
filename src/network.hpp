#pragma once

#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "link.hpp"
#include "fib.hpp"
#include "eqclass.hpp"

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
    std::map<std::string, std::shared_ptr<Node> > nodes;
    std::set<std::shared_ptr<Link>, LinkCompare> links; // all links
    std::set<std::shared_ptr<Link> > failed_links;      // failed links

    std::shared_ptr<FIB> fib;   // the current FIB for this EC
    std::unordered_set<std::shared_ptr<FIB>, FIBHash, FIBEqual> fibs; // history

public:
    Network() = default;
    Network(const std::shared_ptr<cpptoml::table_array>& nodes_config,
            const std::shared_ptr<cpptoml::table_array>& links_config);

    const std::map<std::string, std::shared_ptr<Node> >& get_nodes() const;
    const std::set<std::shared_ptr<Link>, LinkCompare>& get_links() const;

    // The failure agent/process is not implemented yet, but if a link fails,
    // the FIB needs to be recomputed. (A link failure will change the
    // "active_peers" of the related nodes.)
    void compute_fib(const std::shared_ptr<EqClass>& ec);
};
