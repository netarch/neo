#pragma once

#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible communications starting from any of start_nodes, with
 * destination address within pkt_dst, the packets of that communication should
 * eventually be accepted by one of final_nodes when reachable is true.
 * Otherwise, if reachable is false, the packets should either be dropped, or be
 * accepted by none of the final_nodes.
 */
class ReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes;
    bool reachable;

public:
    ReachabilityPolicy(const std::shared_ptr<cpptoml::table>&, const Network&,
                       bool correlated = false);

    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
