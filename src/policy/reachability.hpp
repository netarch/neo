#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified connections, the packets should eventually be accepted by
 * one of target_nodes when reachable is true. Otherwise, if reachable is false,
 * the packets should either be dropped, or be accepted by none of the
 * target_nodes.
 */
class ReachabilityPolicy : public Policy {
private:
    std::unordered_set<Node *> target_nodes;
    bool reachable;

private:
    friend class Config;
    ReachabilityPolicy(bool correlated = false) : Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *, const Network *) override;
    int check_violation(State *) override;
};
