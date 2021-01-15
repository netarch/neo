#pragma once

#include <string>
#include <memory>

#include "lib/ip.hpp"
#include "eqclass.hpp"

class Route
{
private:
    IPNetwork<IPv4Address> network;
    /*
     * If egress_intf is not empty, it means this route is a connected route (or
     * an Openflow update), and the packet should be routed toward the
     * interface, in which case, the next_hop variable does not affect the
     * routing decision.
     */
    IPv4Address next_hop;
    std::string egress_intf;
    int adm_dist;

    // TODO int metric;
    // There's no metric for static routes. The metric can be implemented later
    // if dynamic routing protocols (OSPF, BGP, etc.) are going to be supported.
    // For now, all the installed routes will be regarded as having the same
    // metric value.

    friend bool operator< (const Route&, const Route&);
    friend bool operator<=(const Route&, const Route&);
    friend bool operator> (const Route&, const Route&);
    friend bool operator>=(const Route&, const Route&);
    friend bool operator==(const Route&, const Route&);
    friend bool operator!=(const Route&, const Route&);

private:
    friend class Config;
    Route(): adm_dist(255) {}   // default administrative distance

public:
    Route(const Route&) = default;
    Route(Route&&) = default;
    Route(const IPNetwork<IPv4Address>& net,
          const IPv4Address& nh = IPv4Address(),
          const std::string& ifn = "",
          int adm_dist = 255);

    std::string to_string() const;
    const IPNetwork<IPv4Address>& get_network() const;
    const IPv4Address& get_next_hop() const;
    const std::string& get_intf() const;
    int get_adm_dist() const;
    bool has_same_path(const Route&) const;
    bool relevant_to_ec(const EqClass&) const;

    Route& operator=(const Route&) = default;
    Route& operator=(Route&&) = default;
};

/*
 * Precedence:
 * network (longest prefix)
 * network (network.addr())
 */
bool operator< (const Route&, const Route&);    // this precede other
bool operator<=(const Route&, const Route&);
bool operator> (const Route&, const Route&);    // other precede this
bool operator>=(const Route&, const Route&);
bool operator==(const Route&, const Route&);    // same network (destination)
bool operator!=(const Route&, const Route&);
