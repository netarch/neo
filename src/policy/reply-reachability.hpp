#pragma once

#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible communications starting from any of start_nodes, with
 * destination address within pkt_dst, if the request packet is accepted by one
 * of query_nodes, the reply packet, with its destination address being the
 * source of the request, should be accepted by the sender of request when
 * reachable is true, and when reachable is false, the reply packet should
 * either be dropped, or be accepted by any other node.
 *
 * If the original request packets are not accepted by any of query_nodes, the
 * policy holds.
 */
class ReplyReachabilityPolicy : public Policy
{
private:
    std::unordered_set<Node *> query_nodes;
    bool reachable;

    void parse_query_node(const std::shared_ptr<cpptoml::table>&,
                          const Network&);
    void parse_reachable(const std::shared_ptr<cpptoml::table>&);

public:
    ReplyReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                            const Network&, bool correlated = false);

    std::string to_string() const override;
    void init(State *) const override;
    void check_violation(State *) override;
};