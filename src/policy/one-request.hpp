#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified communications, only one of the requests of those
 * communications should eventually be accepted by one of server_nodes.
 */
class OneRequestPolicy : public Policy
{
private:
    std::unordered_set<Node *> server_nodes;

private:
    friend class Config;
    OneRequestPolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *, const Network *) const override;
    int check_violation(State *) override;
};
