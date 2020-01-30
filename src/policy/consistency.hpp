#pragma once

#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"

class ConsistencyPolicy : public Policy
{
private:
    bool first_run;
    bool result;

public:
    ConsistencyPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);

    std::string to_string() const override;
    void init(State *) const override;
    void check_violation(State *) override;
};
