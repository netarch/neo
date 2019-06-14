#pragma once

#include <string>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;
    IPv4Address next_hop;
    int adm_dist;
    std::string ifname;
    // TODO int metric;
    // There's no metric for static routes. The metric can be implemented later
    // if dynamic routing protocols (OSPF, BGP, etc.) are going to be supported.
    // For now, all the installed routes will be regarded as having the same
    // metrics.

public:
    Route() = delete;
    Route(const Route&) = default;
    Route(Route&&) = default;
    Route(const std::shared_ptr<cpptoml::table>&);
    Route(const IPNetwork<IPv4Address>& net,
          const IPv4Address& nh,
          int adm_dist = 255,
          const std::string& ifn = "");
    Route(const std::string& net,
          const std::string& nh,
          int adm_dist = 255,
          const std::string& ifn = "");

    std::string to_string() const;
    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    int get_adm_dist() const;
    std::string get_ifname() const;
    void set_adm_dist(int dist);

    /*
     * Precedence:
     * network (longest prefix)
     * network (network.addr())
     */
    bool operator< (const Route&) const;    // this precede other
    bool operator<=(const Route&) const;
    bool operator> (const Route&) const;    // other precede this
    bool operator>=(const Route&) const;
    bool operator==(const Route&) const;    // same network (destination)
    bool operator!=(const Route&) const;
    bool has_same_path(const Route&) const;
    Route& operator=(const Route&) = default;
    Route& operator=(Route&&) = default;
};
