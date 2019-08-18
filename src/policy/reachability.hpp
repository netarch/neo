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
 */
class ReachabilityPolicy : public Policy
{
private:
    std::vector<Node *> start_nodes;
    std::vector<Node *> final_nodes;
    bool reachable;

public:
    ReachabilityPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    std::string get_type() const override;
    void config_procs(State *, ForwardingProcess&) const override;
    void check_violation(State *) override;
    void report(State *) const override;
};
