#pragma once

#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible communications starting from any of start_nodes, with
 * destination address within pkt_dst, there should be a communication
 * that reaches each one of final_nodes.
 */

class LoadBalancePolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes, visited;

    void parse_final_node(const std::shared_ptr<cpptoml::table>&,
                          const Network&);

public:
    LoadBalancePolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    void init(State *) const override;
    void check_violation(State *) override;
    ~LoadBalancePolicy();
};
