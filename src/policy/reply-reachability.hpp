#pragma once

#include <unordered_set>

#include "policy/policy.hpp"
class Node;
struct State;

/*
 * For the specified communications, if the request packet is accepted by one of
 * query_nodes, the reply packet, with its destination address being the source
 * of the request, should be accepted by the sender of request when reachable is
 * true, and when reachable is false, the reply packet should either be dropped,
 * or be accepted by any other node.
 *
 * If the original request packets are not accepted by any of query_nodes, the
 * policy fails by default. So the policy implies the ReachabilityPolicy of the
 * requests.
 */
class ReplyReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> query_nodes;
    bool reachable;

private:
    friend class Config;
    ReplyReachabilityPolicy(bool correlated = false): Policy(correlated) {}

public:
    std::string to_string() const override;
    void init(State *, const Network *) const override;
    int check_violation(State *) override;
};
