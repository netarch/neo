#pragma once

#include <vector>
#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from any of start_nodes, with destination
 * address within pkt_dst, the packet should eventually pass through one of
 * waypoints when pass_through is true. Otherwise, if pass_through is false, the
 * packet should not pass through any of the waypoints.
 */
class WaypointPolicy : public Policy
{
private:
    std::unordered_set<Node *> waypoints;
    bool pass_through;

public:
    WaypointPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    const EqClasses& get_ecs() const override;
    size_t num_ecs() const override;
    void compute_ecs(const EqClasses&) override;
    std::string to_string() const override;
    void init(State *) const override;
    void check_violation(State *) override;
};
