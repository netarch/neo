#pragma once

#include <vector>
#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from any of start_nodes, with destination
 * address within pkt_dst, if a packet is accepted by one of query_nodes, the
 * reply packet, with its destination address being the source of the request
 * packet, should be accepted by the sender of request when reachable is true,
 * and when reachable is false, the reply packet should either be dropped, or be
 * accepted by any other node.
 *
 * If the original request packet is not accepted by any of query_nodes, the
 * policy holds.
 */
class ReplyReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> query_nodes;
    bool reachable;
    Node *queried_node;

public:
    ReplyReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                            const Network&);

    std::string to_string() const override;
    void init(State *) const override;
    //void init_procs(State *, const Network&, ForwardingProcess&) const override;
    void check_violation(State *) override;
};
