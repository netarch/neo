#include "route.hpp"
#include "lib/logger.hpp"

Route::Route(const std::shared_ptr<cpptoml::table>& config): adm_dist(255)
{
    auto net = config->get_as<std::string>("network");
    auto nhop = config->get_as<std::string>("next_hop");
    auto dist = config->get_as<int>("adm_dist");

    if (!net) {
        Logger::get_instance().err("Missing network");
    }
    if (!nhop) {
        Logger::get_instance().err("Missing next hop");
    }
    network = IPNetwork<IPv4Address>(*net);
    next_hop = IPv4Address(*nhop);
    if (dist) {
        if (*dist < 0 || *dist > 255) {
            Logger::get_instance().err("Invalid administrative distance: " +
                                       std::to_string(*dist));
        }
        adm_dist = *dist;
    }
}

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             int adm_dist, const std::string& ifn)
    : network(net), next_hop(nh), adm_dist(adm_dist), ifname(ifn)
{
}
Route::Route(const std::string& net, const std::string& nh,
             int adm_dist, const std::string& ifn)
    : network(net), next_hop(nh), adm_dist(adm_dist), ifname(ifn)
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

int Route::get_adm_dist() const
{
    return adm_dist;
}

std::string Route::get_ifname() const
{
    return ifname;
}

void Route::set_adm_dist(int dist)
{
    adm_dist = dist;
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
