#pragma once

#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"

/*
 * For all possible packets starting from any of start_nodes, with source and
 * destination addresses within pkt_src and pkt_dst, respectively, the packet
 * will eventually pass through one of waypoints when pass_through is true.
 * Otherwise, if pass_through is false, the packet will not pass through any of
 * the waypoints, which means none of the waypoints will be on the forwarding
 * path(s) of the packet.
 */
class WaypointPolicy : public Policy
{
private:
    std::vector<Node *> start_nodes;
    std::vector<Node *> waypoints;
    bool pass_through;

public:
    WaypointPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    std::string get_type() const override;
    void init() override;
    void config_procs(State *, ForwardingProcess&) const override;
    void check_violation(State *) override;
};
