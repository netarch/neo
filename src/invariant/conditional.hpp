#pragma once

#include "invariant/invariant.hpp"

/**
 * If the first correlated invariant holds, the conditional invariant holds if
 * all the remaining correlated invariants hold. If the first correlated
 * invariant does not hold, the conditional invariant holds.
 */
class Conditional : public Invariant {
private:
    bool result;

private:
    friend class ConfigParser;
    Conditional() = default;

public:
    std::string to_string() const override;
    void init() override;
    void reinit() override;
    int check_violation() override;
};
