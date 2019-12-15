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
    std::vector<Node *> start_nodes;
    std::unordered_set<Node *> query_nodes;
    bool reachable;
    EqClasses pre_ECs;  // request ECs (prerequisite ECs)
    EqClasses ECs;      // reply ECs (ECs to be verified)
    Node *queried_node;

public:
    ReplyReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                            const Network&);

    const EqClasses& get_pre_ecs() const override;
    const EqClasses& get_ecs() const override;
    size_t num_ecs() const override;
    void compute_ecs(const EqClasses&) override;
    std::string to_string() const override;
    std::string get_type() const override;
    void init(State *) override;
    void config_procs(State *, const Network&, ForwardingProcess&) const
    override;
    void check_violation(State *) override;
};
