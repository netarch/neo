#pragma once

#include <vector>
#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from any of start_nodes, with destination
 * address within pkt_dst, the packet should eventually be accepted by one of
 * final_nodes when reachable is true. Otherwise, if reachable is false, the
 * packet should either be dropped, or be accepted by none of the final_nodes.
 */
class ReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes;
    bool reachable;

public:
    ReachabilityPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    void init(State *) const override;
    void check_violation(State *) override;
};
