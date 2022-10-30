#pragma once

#include "policy/policy.hpp"

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
    void init() override;
    void reinit() override;
    int check_violation() override;
};
