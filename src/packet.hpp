#pragma once

#include <functional>

#include "interface.hpp"
#include "lib/ip.hpp"
#include "lib/hash.hpp"

/*
 * A located abstract packet.
 */
class Packet
{
private:
    Interface *interface;
    IPv4Address src_ip, dst_ip;

    friend struct PacketHash;
    friend bool operator==(const Packet&, const Packet&);

public:
    Packet(Interface *, const IPv4Address& src_ip, const IPv4Address& dst_ip);

    Interface *get_intf() const;
    IPv4Address get_src_ip() const;
    IPv4Address get_dst_ip() const;
};

bool operator==(const Packet&, const Packet&);

struct PacketHash {
    size_t operator()(Packet *const& p) const
    {
        size_t value = 0;
        std::hash<IPv4Address> ipv4_hf;
        std::hash<Interface *> intf_hf;
        ::hash::hash_combine(value, ipv4_hf(p->src_ip));
        ::hash::hash_combine(value, ipv4_hf(p->dst_ip));
        ::hash::hash_combine(value, intf_hf(p->interface));
        return value;
    }
};

struct PacketEq {
    bool operator()(Packet *const& a, Packet *const& b) const
    {
        return *a == *b;
    }
};
