#pragma once

#include <vector>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from any of start_nodes, with source and
 * destination addresses within pkt_src and pkt_dst, respectively, the packet
 * will eventually be accepted by one of final_nodes when reachable is true.
 * Otherwise, if reachable is false, the packet will either be dropped, or be
 * accepted by none of the final_nodes.
 *
 * The policy states that the above is true when the prerequisite policy is
 * verified to be true. If the prerequisite policy is violated, the policy
 * always holds.
 */
class StatefulReachabilityPolicy : public Policy
{
private:
    std::vector<Node *> start_nodes;
    std::vector<Node *> final_nodes;
    bool reachable;
    Policy *prerequisite;

public:
    StatefulReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                               const Network&);
    ~StatefulReachabilityPolicy() override;

    const EqClasses& get_pre_ecs() const override;
    size_t num_ecs() const override;
    void compute_ecs(const EqClasses&) override;
    std::string to_string() const override;
    std::string get_type() const override;
    void init(State *) override;
    void config_procs(State *, ForwardingProcess&) const override;
    void check_violation(State *) override;
};
