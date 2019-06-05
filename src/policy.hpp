#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

class Policy
{
protected:
    ;

public:
    Policy() = default;
    Policy(const Policy&) = default;

    virtual void load_config(const std::shared_ptr<cpptoml::table>&) {}
};
