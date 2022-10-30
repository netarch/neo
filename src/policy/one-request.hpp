#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;

/*
 * For the specified connections, only one of the requests of those connections
 * should eventually be accepted by one of target_nodes.
 */
class OneRequestPolicy : public Policy {
private:
    std::unordered_set<Node *> target_nodes;

private:
    friend class Config;
    OneRequestPolicy(bool correlated = false) : Policy(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
