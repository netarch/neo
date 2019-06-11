#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "network.hpp"
#include "process/forwarding.hpp"

class Policy
{
public:
    Policy() = default;

    virtual void load_config(const std::shared_ptr<cpptoml::table>&,
                             const Network&);
    virtual bool check_violation(const Network&, const ForwardingProcess&);
};
