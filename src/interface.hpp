#pragma once

#include <string>
#include <memory>

#include "lib/ip.hpp"

class Interface
{
private:
    std::string name;
    IPInterface<IPv4Address> ipv4;
    bool L3;

public:
    Interface(const std::string& name);
    Interface(const std::string& name, const std::string& intf);

    std::string get_name() const;
    IPv4Address addr() const;
    int prefix_length() const;
    IPNetwork<IPv4Address> network() const;
    bool switching() const;
};
