#pragma once

#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible communications starting from any of start_nodes, with
 * destination address within pkt_dst, there should be communications reaching
 * each one of final_nodes within the given number of repetitions of
 * incrementing the source port number by one.
 */
class LoadBalancePolicy : public Policy
{
private:
    std::unordered_set<Node *> final_nodes, visited;
    int repetition;

public:
    LoadBalancePolicy(const std::shared_ptr<cpptoml::table>&, const Network&,
                      bool correlated = false);

    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
