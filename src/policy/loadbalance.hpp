#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified communications, there should be communications reaching
 * each one of final_nodes within the given number of repetitions of
 * incrementing the source port number by one.
 */
class LoadBalancePolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes, visited;
    int repetition;

private:
    friend class Config;
    LoadBalancePolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *, const Network *) const override;
    int check_violation(State *) override;
};
