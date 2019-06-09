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
    // TODO int metric;
    // There's no metric for static routes. The metric can be implemented later
    // if dynamic routing protocols (OSPF, BGP, etc.) are going to be supported.
    // For now, all the installed routes will be regarded as having the same
    // metrics.

public:
    Route() = delete;
    Route(const Route&) = default;
    Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh);
    Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
          int adm_dist);
    Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
          const std::string& ifn);
    Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
          const std::string& ifn, int adm_dist);
    Route(const std::string& net, const std::string& nh);
    Route(const std::string& net, const std::string& nh, int adm_dist);
    Route(const std::string& net, const std::string& nh,
          const std::string& ifn);
    Route(const std::string& net, const std::string& nh, const std::string& ifn,
          int adm_dist);

    std::string to_string() const;
    IPNetwork<IPv4Address> get_network() const;
    IPv4Address get_next_hop() const;
    std::string get_ifname() const;
    int get_adm_dist() const;
    void set_next_hop(const IPv4Address&);
    void set_next_hop(const std::string&);
    void set_ifname(const std::string&);

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
    Route& operator=(const Route&);
};
