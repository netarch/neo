#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "network.hpp"

class Policy
{
public:
    Policy() = default;
    Policy(const Policy&) = default;

    virtual void load_config(const std::shared_ptr<cpptoml::table>&);
    //virtual bool check_violation();
};
