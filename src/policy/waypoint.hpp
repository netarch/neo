#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified communications, the packets of that communication should
 * eventually pass through one of waypoints if pass_through is true. Otherwise,
 * if pass_through is false, the packet should not pass through any of the
 * waypoints.
 */
class WaypointPolicy : public Policy
{
private:
    std::unordered_set<Node *> waypoints;
    bool pass_through;

private:
    friend class Config;
    WaypointPolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
