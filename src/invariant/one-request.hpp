#pragma once

#include <unordered_set>

#include "invariant/invariant.hpp"

class Node;

/**
 * For the specified connections, only one of the requests of those connections
 * should eventually be accepted by one of target_nodes.
 */
class OneRequest : public Invariant {
private:
    std::unordered_set<Node *> target_nodes;

private:
    friend class ConfigParser;
    OneRequest(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
