#pragma once

#include <unordered_set>

#include "invariant/invariant.hpp"

class Node;

/**
 * For the specified connections, the packets should eventually be accepted by
 * one of target_nodes when reachable is true. Otherwise, if reachable is false,
 * the packets should either be dropped, or be accepted by none of the
 * target_nodes.
 */
class Reachability : public Invariant {
private:
    std::unordered_set<Node *> target_nodes;
    bool reachable;

private:
    friend class ConfigParser;
    Reachability(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
