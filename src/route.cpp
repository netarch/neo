#include "route.hpp"
#include "lib/logger.hpp"

Route::Route(const std::shared_ptr<cpptoml::table>& config): adm_dist(255)
{
    auto net = config->get_as<std::string>("network");
    auto nhop = config->get_as<std::string>("next_hop");
    auto intf = config->get_as<std::string>("interface");
    auto dist = config->get_as<int>("adm_dist");

    if (!net) {
        Logger::get_instance().err("Missing network");
    }
    if (!nhop && !intf) {
        Logger::get_instance().err("Missing next hop IP address or interface");
    }

    network = IPNetwork<IPv4Address>(*net);
    if (nhop) {
        next_hop = IPv4Address(*nhop);
    }
    if (intf) {
        egress_intf = *intf;
    }
    if (dist) {
        if (*dist < 1 || *dist > 254) {
            Logger::get_instance().err("Invalid administrative distance: " +
                                       std::to_string(*dist));
        }
        adm_dist = *dist;
    }
}

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             const std::string& ifn, int adm_dist)
    : network(net), next_hop(nh), egress_intf(ifn), adm_dist(adm_dist)
{
}
Route::Route(const std::string& net, const std::string& nh,
             const std::string& ifn, int adm_dist)
    : network(net), next_hop(nh), egress_intf(ifn), adm_dist(adm_dist)
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

std::string Route::get_intf() const
{
    return egress_intf;
}

int Route::get_adm_dist() const
{
    return adm_dist;
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
