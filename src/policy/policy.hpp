#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "network.hpp"
#include "process/forwarding.hpp"

class Policy
{
public:
    virtual bool check_violation(const Network&, const ForwardingProcess&);
};
