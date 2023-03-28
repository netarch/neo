#pragma once

#include <unordered_set>

#include "invariant/invariant.hpp"

class Node;

/**
 * For the specified connections, the dispersion index (VMR) of the balanced
 * requests across the target_nodes is less than or equal to the maximum at all
 * times.
 */
class LoadBalance : public Invariant {
private:
    std::unordered_set<Node *> target_nodes;
    double max_dispersion_index; // variance-to-mean ratio (VMR)

private:
    friend class ConfigParser;
    LoadBalance(bool correlated = false) : Invariant(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
