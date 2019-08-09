#pragma once

#include <vector>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "lib/ip.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from start_node, with source and
 * destination addresses within pkt_src and pkt_dst, respectively, the packet
 * will eventually be accepted by final_node when reachable is true. Otherwise,
 * if reachable is false, the packet will either be dropped by final_node, or
 * never reach final_node.
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
    //bool check_violation(const Network&, const ForwardingProcess&) override;
};
