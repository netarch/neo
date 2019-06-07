#include "route.hpp"

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh)
    : network(net), next_hop(nh), adm_dist(255)
{
}

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             int adm_dist)
    : network(net), next_hop(nh), adm_dist(adm_dist)
{
}

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             const std::string& ifn)
    : network(net), next_hop(nh), ifname(ifn), adm_dist(255)
{
}

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             const std::string& ifn, int adm_dist)
    : network(net), next_hop(nh), ifname(ifn), adm_dist(adm_dist)
{
}

Route::Route(const std::string& net, const std::string& nh)
    : network(net), next_hop(nh), adm_dist(255)
{
}

Route::Route(const std::string& net, const std::string& nh, int adm_dist)
    : network(net), next_hop(nh), adm_dist(adm_dist)
{
}

Route::Route(const std::string& net, const std::string& nh,
             const std::string& ifn)
    : network(net), next_hop(nh), ifname(ifn), adm_dist(255)
{
}

Route::Route(const std::string& net, const std::string& nh,
             const std::string& ifn, int adm_dist)
    : network(net), next_hop(nh), ifname(ifn), adm_dist(adm_dist)
{
}

std::string Route::to_string() const
{
    return network.to_string() + " --> " + next_hop.to_string();
}

IPNetwork<IPv4Address> Route::get_network() const
{
    return network;
}

IPv4Address Route::get_next_hop() const
{
    return next_hop;
}

std::string Route::get_ifname() const
{
    return ifname;
}

int Route::get_adm_dist() const
{
    return adm_dist;
}

void Route::set_next_hop(const IPv4Address& nhop)
{
    next_hop = nhop;
}

void Route::set_next_hop(const std::string& nhop)
{
    next_hop = nhop;
}

void Route::set_ifname(const std::string& ifn)
{
    ifname = ifn;
}

bool Route::operator<(const Route& rhs) const
{
    if (network.prefix_length() > rhs.network.prefix_length()) {
        return true;
    } else if (network.prefix_length() < rhs.network.prefix_length()) {
        return false;
    }
    return network.addr() < rhs.network.addr();
}

bool Route::operator<=(const Route& rhs) const
{
    if (network.prefix_length() > rhs.network.prefix_length()) {
        return true;
    } else if (network.prefix_length() < rhs.network.prefix_length()) {
        return false;
    }
    return network.addr() <= rhs.network.addr();
}

bool Route::operator>(const Route& rhs) const
{
    if (network.prefix_length() > rhs.network.prefix_length()) {
        return false;
    } else if (network.prefix_length() < rhs.network.prefix_length()) {
        return true;
    }
    return network.addr() > rhs.network.addr();
}

bool Route::operator>=(const Route& rhs) const
{
    if (network.prefix_length() > rhs.network.prefix_length()) {
        return false;
    } else if (network.prefix_length() < rhs.network.prefix_length()) {
        return true;
    }
    return network.addr() >= rhs.network.addr();
}

bool Route::operator==(const Route& rhs) const
{
    if (network == rhs.network) {
        return true;
    }
    return false;
}

bool Route::operator!=(const Route& rhs) const
{
    return !(*this == rhs);
}

bool Route::has_same_path(const Route& other) const
{
    if (network == other.network && next_hop == other.next_hop) {
        return true;
    }
    return false;
}

Route& Route::operator=(const Route& rhs)
{
    network = rhs.network;
    next_hop = rhs.next_hop;
    ifname = rhs.ifname;
    adm_dist = rhs.adm_dist;
    return *this;
}
