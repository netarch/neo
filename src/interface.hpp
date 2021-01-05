#pragma once

#include <string>

#include "lib/ip.hpp"

class Interface
{
private:
    std::string name;
    IPInterface<IPv4Address> ipv4;
    bool is_switchport;

private:
    friend class Config;
    Interface() = default;

public:
    std::string to_string() const;
    std::string get_name() const;
    int prefix_length() const;
    IPv4Address addr() const;
    IPv4Address mask() const;
    IPNetwork<IPv4Address> network() const;
    bool is_l2() const;
};
