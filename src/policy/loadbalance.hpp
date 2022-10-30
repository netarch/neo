#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;

/*
 * For the specified connections, the dispersion index (VMR) of the balanced
 * requests across the target_nodes is less than or equal to the maximum at all
 * times.
 */
class LoadBalancePolicy : public Policy {
private:
    std::unordered_set<Node *> target_nodes;
    double max_dispersion_index; // variance-to-mean ratio (VMR)

private:
    friend class Config;
    LoadBalancePolicy(bool correlated = false) : Policy(correlated) {}

public:
    std::string to_string() const override;
    void init() override;
    int check_violation() override;
};
