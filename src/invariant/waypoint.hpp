#pragma once

#include <unordered_set>

#include "invariant/invariant.hpp"

class Node;

/**
 * For the specified connections, the packets should eventually pass through one
 * of target_nodes if pass_through is true. Otherwise, if pass_through is false,
 * the packet should not pass through any of the target_nodes.
 */
class Waypoint : public Invariant {
private:
    std::unordered_set<Node *> target_nodes;
    bool pass_through;

private:
    friend class ConfigParser;
    Waypoint(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
