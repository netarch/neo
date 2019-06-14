#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"

class StatefulReachabilityPolicy : public Policy
{
private:
    ;

public:
    StatefulReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                               const Network&);
};
