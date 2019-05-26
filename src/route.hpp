#pragma once

#include <string>

#include "lib/ip.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;
    IPv4Address next_hop;
    std::string ifname;
    int adm_dist;

public:
    Route() = delete;
    Route(const Route&) = default;
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&);
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&, int adm_dist);
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&,
          const std::string&);
    Route(const IPNetwork<IPv4Address>&, const IPv4Address&,
          const std::string&, int adm_dist);
    Route(const std::string&, const std::string&);
    Route(const std::string&, const std::string&, int adm_dist);
    Route(const std::string&, const std::string&, const std::string&);
    Route(const std::string&, const std::string&, const std::string&,
          int adm_dist);

    std::string to_string() const;
    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    std::string get_ifname() const;
    int get_adm_dist() const;
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
    Route& operator=(const Route&);
};
