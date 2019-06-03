#pragma once

#include <string>

#include "node.hpp"

class Middlebox : public Node
{
private:
    ;

public:
    Middlebox() = delete;
    Middlebox(const Middlebox&) = default;
    Middlebox(const std::string&);
};
