#pragma once

#include <string>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;

    /*
     * If egress_intf is not empty, it means this route is a connected route,
     * and the packet should be routed toward the interface, in which case, the
     * next_hop variable does not affect the routing decision.
     */
    IPv4Address next_hop;
    std::string egress_intf;

    int adm_dist;

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
          const IPv4Address& nh = IPv4Address(),
          const std::string& ifn = "",
          int adm_dist = 255);
    Route(const std::string& net,
          const std::string& nh = std::string(),
          const std::string& ifn = "",
          int adm_dist = 255);

    std::string to_string() const;
    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    std::string get_intf() const;
    int get_adm_dist() const;
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
