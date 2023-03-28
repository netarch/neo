#pragma once

#include <unordered_set>

#include "invariant/invariant.hpp"

class Node;

/**
 * For the specified connections, both the request and reply packets should be
 * accepted by one of target_nodes when reachable is true.
 */
class ReplyReachability : public Invariant {
private:
    std::unordered_set<Node *> target_nodes;
    bool reachable;

private:
    friend class ConfigParser;
    ReplyReachability(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
