#pragma once

#include "lib/ip.hpp"

class EqClass : public IPRange<IPv4Address>
{
private:
    // set (?) of std::shared_ptr<EqClass> depECs;

public:
    EqClass();
};
