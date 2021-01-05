#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For the specified communications, the packets of that communication should
 * eventually be accepted by one of final_nodes when reachable is true.
 * Otherwise, if reachable is false, the packets should either be dropped, or be
 * accepted by none of the final_nodes.
 */
class ReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes;
    bool reachable;

private:
    friend class Config;
    ReachabilityPolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
