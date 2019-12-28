#pragma once

#include <functional>

#include "interface.hpp"
#include "lib/ip.hpp"
#include "lib/hash.hpp"

/*
 * Packet state
 */
#define PS_TCP_INIT_1   0   // TCP 3-way handshake SYN
#define PS_TCP_INIT_2   1   // TCP 3-way handshake SYN/ACK
#define PS_TCP_INIT_3   2   // TCP 3-way handshake ACK
#define PS_HTTP_REQ     3   // HTTP request
#define PS_HTTP_REQ_A   4   // HTTP request ACK
#define PS_HTTP_REP     5   // HTTP reply
#define PS_HTTP_REP_A   6   // HTTP reply ACK
#define PS_TCP_TERM_1   7   // TCP termination FIN/ACK
#define PS_TCP_TERM_2   8   // TCP termination FIN/ACK
#define PS_TCP_TERM_3   9   // TCP termination ACK
#define PS_ICMP_REQ    10   // ICMP request
#define PS_ICMP_REP    11   // ICMP reply

#define PS_IS_REQUEST(x) (      \
        (x) == PS_TCP_INIT_1 || \
        (x) == PS_TCP_INIT_3 || \
        (x) == PS_HTTP_REQ   || \
        (x) == PS_HTTP_REP_A || \
        (x) == PS_TCP_TERM_2 || \
        (x) == PS_ICMP_REQ      \
)

#define PS_IS_REPLY(x)  (       \
        (x) == PS_TCP_INIT_2 || \
        (x) == PS_HTTP_REQ_A || \
        (x) == PS_HTTP_REP   || \
        (x) == PS_TCP_TERM_1 || \
        (x) == PS_TCP_TERM_3 || \
        (x) == PS_ICMP_REP      \
)

/*
 * A located abstract representative packet.
 */
class Packet
{
private:
    Interface *interface;
    IPv4Address src_ip, dst_ip;
    uint16_t src_port, dst_port;
    uint8_t pkt_state;

    friend struct PacketHash;
    friend bool operator==(const Packet&, const Packet&);

public:
    Packet(Interface *, const IPv4Address& src_ip, const IPv4Address& dst_ip,
           uint16_t src_port, uint16_t dst_port, uint8_t pkt_state);

    Interface *get_intf() const;
    IPv4Address get_src_ip() const;
    IPv4Address get_dst_ip() const;
    uint16_t get_src_port() const;
    uint16_t get_dst_port() const;
    uint8_t get_pkt_state() const;
};

bool operator==(const Packet&, const Packet&);

struct PacketHash {
    size_t operator()(Packet *const& p) const
    {
        size_t value = 0;
        std::hash<Interface *> intf_hf;
        std::hash<IPv4Address> ipv4_hf;
        std::hash<uint16_t>    port_hf;
        std::hash<uint8_t>     state_hf;
        ::hash::hash_combine(value, intf_hf(p->interface));
        ::hash::hash_combine(value, ipv4_hf(p->src_ip));
        ::hash::hash_combine(value, ipv4_hf(p->dst_ip));
        ::hash::hash_combine(value, port_hf(p->src_port));
        ::hash::hash_combine(value, port_hf(p->dst_port));
        ::hash::hash_combine(value, state_hf(p->pkt_state));
        return value;
    }
};

struct PacketEq {
    bool operator()(Packet *const& a, Packet *const& b) const
    {
        return *a == *b;
    }
};
