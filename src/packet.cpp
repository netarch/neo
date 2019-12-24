#include "packet.hpp"

Packet::Packet(Interface *intf, const IPv4Address& src_ip,
               const IPv4Address& dst_ip, uint16_t src_port, uint16_t dst_port,
               uint8_t pkt_state)
    : interface(intf), src_ip(src_ip), dst_ip(dst_ip), src_port(src_port),
      dst_port(dst_port), pkt_state(pkt_state)
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

uint16_t Packet::get_src_port() const
{
    return src_port;
}

uint16_t Packet::get_dst_port() const
{
    return dst_port;
}

uint8_t Packet::get_pkt_state() const
{
    return pkt_state;
}

bool operator==(const Packet& a, const Packet& b)
{
    return (a.interface == b.interface &&
            a.src_ip == b.src_ip &&
            a.dst_ip == b.dst_ip &&
            a.src_port == b.src_port &&
            a.dst_port == b.dst_port &&
            a.pkt_state == b.pkt_state);
}
