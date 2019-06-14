#pragma once

#include <cpptoml/cpptoml.hpp>

#include "node.hpp"

class Middlebox : public Node
{
private:
    ;

public:
    Middlebox(const std::shared_ptr<cpptoml::table>&);
};
