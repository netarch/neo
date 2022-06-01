#pragma once

#include "policy/policy.hpp"
struct State;

/*
 * If the first correlated policy holds, the conditional policy holds if all the
 * remaining correlated policies hold. If the first correlated policy does not
 * hold, the conditional policy holds.
 */
class ConditionalPolicy : public Policy {
private:
    bool result;

private:
    friend class Config;
    ConditionalPolicy() = default;

public:
    std::string to_string() const override;
    void init(State *, const Network *) override;
    void reinit(State *, const Network *) override;
    int check_violation(State *) override;
};
