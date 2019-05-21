#include "route.hpp"

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh)
    : network(net), next_hop(nh)
{
}

Route::Route(const std::string& net, const std::string& nh)
    : network(net), next_hop(nh)
{
}

IPNetwork<IPv4Address> Route::get_network() const
{
    return network;
}

IPv4Address Route::get_next_hop() const
{
    return next_hop;
}
