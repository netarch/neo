#pragma once

#include <string>

#include "lib/ip.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;
    IPv4Address next_hop;

public:
    Route() = default;
    Route(const Route&) = default;
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&);
    Route(const std::string&, const std::string&);

    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    //bool operator<(const Route&) const;
};
