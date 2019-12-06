#include "packet.hpp"

Packet::Packet(Interface *intf, const IPv4Address& src_ip,
               const IPv4Address& dst_ip)
    : interface(intf), src_ip(src_ip), dst_ip(dst_ip)
{
}

Interface *Packet::get_intf() const
{
    return interface;
}

IPv4Address Packet::get_src_ip() const
{
    return src_ip;
}

IPv4Address Packet::get_dst_ip() const
{
    return dst_ip;
}

bool operator==(const Packet& a, const Packet& b)
{
    return (a.interface == b.interface &&
            a.src_ip == b.src_ip &&
            a.dst_ip == b.dst_ip);
}
