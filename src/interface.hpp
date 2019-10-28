#pragma once

#include <string>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"

class Interface
{
private:
    std::string name;
    IPInterface<IPv4Address> ipv4;
    bool switchport;
    // TODO std::vector<int> vlans;

public:
    Interface(const std::shared_ptr<cpptoml::table>&);

    std::string to_string() const;
    std::string get_name() const;
    int prefix_length() const;
    IPv4Address addr() const;
    IPv4Address mask() const;
    IPNetwork<IPv4Address> network() const;
    bool is_l2() const;
};
