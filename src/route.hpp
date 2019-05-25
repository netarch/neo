#pragma once

#include <string>

#include "lib/ip.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;
    IPv4Address next_hop;
    std::string ifname;

public:
    Route() = default;
    Route(const Route&) = default;
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&);
    Route(const std::string&, const std::string&);
    Route(const std::string&, const std::string&, const std::string&);

    std::string to_string() const;
    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    std::string get_ifname() const;
    void set_next_hop(const IPv4Address&);
    void set_next_hop(const std::string&);
    void set_ifname(const std::string&);
    bool operator< (const Route&) const;    // this precede other
    bool operator<=(const Route&) const;
    bool operator> (const Route&) const;    // other precede this
    bool operator>=(const Route&) const;
    bool operator==(const Route&) const;
    bool operator!=(const Route&) const;
    bool identical(const Route&) const;
};
