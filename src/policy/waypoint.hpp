#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"

class WaypointPolicy : public Policy
{
private:
    ;

public:
    WaypointPolicy(const std::shared_ptr<cpptoml::table>&, const Network&);
};
