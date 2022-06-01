#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified connections, both the request and reply packets should be
 * accepted by one of target_nodes when reachable is true.
 */
class ReplyReachabilityPolicy : public Policy {
private:
    std::unordered_set<Node *> target_nodes;
    bool reachable;

private:
    friend class Config;
    ReplyReachabilityPolicy(bool correlated = false) : Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *, const Network *) override;
    int check_violation(State *) override;
};
