#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;

/*
 * For the specified connections, the packets should eventually pass through one
 * of target_nodes if pass_through is true. Otherwise, if pass_through is false,
 * the packet should not pass through any of the target_nodes.
 */
class WaypointPolicy : public Policy {
private:
    std::unordered_set<Node *> target_nodes;
    bool pass_through;

private:
    friend class ConfigParser;
    WaypointPolicy(bool correlated = false) : Policy(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
