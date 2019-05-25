#include "route.hpp"

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh)
    : network(net), next_hop(nh)
{
}

Route::Route(const std::string& net, const std::string& nh)
    : network(net), next_hop(nh)
{
}

Route::Route(const std::string& net, const std::string& nh,
             const std::string& ifn)
    : network(net), next_hop(nh), ifname(ifn)
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
    if (network.prefix_length() == rhs.network.prefix_length()
            && network.addr() == rhs.network.addr()) {
        return true;
    }
    return false;
}

bool Route::operator!=(const Route& rhs) const
{
    return !(*this == rhs);
}
