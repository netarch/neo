#pragma once

#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible communications listed, with destination address within
 * pkt_dst's, only one of the requests of those communications should eventually
 * be accepted by one of server_nodes.
 */
class OneRequestPolicy : public Policy
{
private:
    std::unordered_set<Node *> server_nodes;

public:
    OneRequestPolicy(const std::shared_ptr<cpptoml::table>&,
                     const Network&, bool correlated = false);

    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
