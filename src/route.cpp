#include "route.hpp"

#include "lib/logger.hpp"

Route::Route(const IPNetwork<IPv4Address>& net, const IPv4Address& nh,
             const std::string& ifn, int adm_dist)
    : network(net), next_hop(nh), egress_intf(ifn), adm_dist(adm_dist)
{
}

std::string Route::to_string() const
{
    return network.to_string() + " --> " + next_hop.to_string();
}

const IPNetwork<IPv4Address>& Route::get_network() const
{
    return network;
}

const IPv4Address& Route::get_next_hop() const
{
    return next_hop;
}

const std::string& Route::get_intf() const
{
    return egress_intf;
}

int Route::get_adm_dist() const
{
    return adm_dist;
}

bool Route::has_same_path(const Route& other) const
{
    if (network == other.network && next_hop == other.next_hop) {
        return true;
    }
    return false;
}

bool Route::relevant_to_ec(const EqClass& ec) const
{
    bool fully_contained = false;   // whether ec is fully contained within network
    bool has_been_set = false;

    for (const ECRange& range : ec) {
        if (has_been_set) {
            if (this->network.contains(range) != fully_contained) {
                /*
                 * The "network" of this route should either contain all the EC
                 * ranges or none at all.
                 */
                Logger::get().err("Route splits an EC");
            }
        } else {
            // first loop
            fully_contained = this->network.contains(range);
            has_been_set = true;
        }
    }

    return fully_contained;
}

bool operator<(const Route& a, const Route& b)
{
    if (a.network.prefix_length() > b.network.prefix_length()) {
        return true;
    } else if (a.network.prefix_length() < b.network.prefix_length()) {
        return false;
    }
    return a.network.addr() < b.network.addr();
}

bool operator<=(const Route& a, const Route& b)
{
    if (a.network.prefix_length() > b.network.prefix_length()) {
        return true;
    } else if (a.network.prefix_length() < b.network.prefix_length()) {
        return false;
    }
    return a.network.addr() <= b.network.addr();
}

bool operator>(const Route& a, const Route& b)
{
    if (a.network.prefix_length() > b.network.prefix_length()) {
        return false;
    } else if (a.network.prefix_length() < b.network.prefix_length()) {
        return true;
    }
    return a.network.addr() > b.network.addr();
}

bool operator>=(const Route& a, const Route& b)
{
    if (a.network.prefix_length() > b.network.prefix_length()) {
        return false;
    } else if (a.network.prefix_length() < b.network.prefix_length()) {
        return true;
    }
    return a.network.addr() >= b.network.addr();
}

bool operator==(const Route& a, const Route& b)
{
    if (a.network == b.network) {
        return true;
    }
    return false;
}

bool operator!=(const Route& a, const Route& b)
{
    return !(a == b);
}
