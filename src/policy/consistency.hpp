#pragma once

#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"

/*
 * All converged states of all execution paths of all correlated policies should
 * have the same, consistent verification result, either all get verified to be
 * true or violated.
 */
class ConsistencyPolicy : public Policy
{
private:
    bool first_run;
    bool result;

public:
    ConsistencyPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    void init(State *) override;
    int check_violation(State *) override;
};
