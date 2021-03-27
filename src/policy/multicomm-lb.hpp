#pragma once

#include <unordered_set>
#include <unordered_map>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified communications, the dispersion index (VMR) of the balanced
 * requests is less than or equal to the maximum at all times.
 */
class MulticommLBPolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes;
    double max_dispersion_index; // variance-to-mean ratio (VMR)

private:
    friend class Config;
    MulticommLBPolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};

class ReachCounts
{
private:
    std::unordered_map<Node *, int> counts;

    friend class ReachCountsHash;
    friend class ReachCountsEq;

public:
    std::string to_string() const;
    int operator[](Node *const&) const;
    void increase(Node *const&);
};

class ReachCountsHash
{
public:
    size_t operator()(const ReachCounts *const&) const;
};

class ReachCountsEq
{
public:
    bool operator()(const ReachCounts *const&, const ReachCounts *const&) const;
};
