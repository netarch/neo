#pragma once

#include "policy/policy.hpp"
struct State;

/*
 * All converged states of all execution paths of all correlated policies should
 * have the same, consistent verification result, either all get verified to be
 * true or violated.
 */
class ConsistencyPolicy : public Policy
{
private:
    bool result;

private:
    friend class Config;
    ConsistencyPolicy() = default;

public:
    std::string to_string() const override;
    void init(State *, const Network *) const override;
    void reinit(State *, const Network *) const override;
    int check_violation(State *) override;
};
