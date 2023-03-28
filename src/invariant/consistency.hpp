#pragma once

#include "invariant/invariant.hpp"

/**
 * All converged states of all execution paths of all correlated invariants
 * should have the same, consistent verification result, either all get verified
 * to be true or violated.
 */
class Consistency : public Invariant {
private:
    bool result, unset;

private:
    friend class ConfigParser;
    Consistency() = default;

public:
    std::string to_string() const override;
    void init() override;
    void reinit() override;
    int check_violation() override;
};
