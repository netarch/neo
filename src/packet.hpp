#pragma once

#include <string>
#include <functional>

#include "interface.hpp"
#include "eqclass.hpp"
#include "payload.hpp"
#include "policy/policy.hpp"
#include "lib/ip.hpp"
#include "lib/hash.hpp"
class State;

/*
 * Packet state
 */
#define PS_TCP_INIT_1       0   // TCP 3-way handshake SYN
#define PS_TCP_INIT_2       1   // TCP 3-way handshake SYN/ACK
#define PS_TCP_INIT_3       2   // TCP 3-way handshake ACK
#define PS_HTTP_REQ         3   // HTTP request
#define PS_HTTP_REQ_A       4   // HTTP request ACK
#define PS_HTTP_REP         5   // HTTP reply
#define PS_HTTP_REP_A       6   // HTTP reply ACK
#define PS_TCP_TERM_1       7   // TCP termination FIN/ACK
#define PS_TCP_TERM_2       8   // TCP termination FIN/ACK
#define PS_TCP_TERM_3       9   // TCP termination ACK
#define PS_ICMP_ECHO_REQ   10   // ICMP echo request
#define PS_ICMP_ECHO_REP   11   // ICMP echo reply

#define PS_IS_REQUEST(x) (      \
        (x) == PS_TCP_INIT_1 || \
        (x) == PS_TCP_INIT_3 || \
        (x) == PS_HTTP_REQ   || \
        (x) == PS_HTTP_REP_A || \
        (x) == PS_TCP_TERM_2 || \
        (x) == PS_ICMP_ECHO_REQ \
)
#define PS_IS_REPLY(x) (        \
        (x) == PS_TCP_INIT_2 || \
        (x) == PS_HTTP_REQ_A || \
        (x) == PS_HTTP_REP   || \
        (x) == PS_TCP_TERM_1 || \
        (x) == PS_TCP_TERM_3 || \
        (x) == PS_ICMP_ECHO_REP \
)
#define PS_IS_FIRST(x) (        \
        (x) == PS_TCP_INIT_1 || \
        (x) == PS_ICMP_ECHO_REQ \
)
#define PS_IS_TCP(x)       ((x) >= PS_TCP_INIT_1 && (x) <= PS_TCP_TERM_3)
#define PS_IS_ICMP_ECHO(x) ((x) >= PS_ICMP_ECHO_REQ && (x) <= PS_ICMP_ECHO_REP)
#define PS_HAS_SYN(x)      ((x) == PS_TCP_INIT_1 || (x) == PS_TCP_INIT_2)
#define PS_HAS_FIN(x)      ((x) == PS_TCP_TERM_1 || (x) == PS_TCP_TERM_2)
#define PS_SAME_PROTO(x, y) (                       \
        (PS_IS_TCP(x) && PS_IS_TCP(y)) ||           \
        (PS_IS_ICMP_ECHO(x) && PS_IS_ICMP_ECHO(y))  \
)
#define PS_IS_NEXT(x, y) (  \
        (x) == (y) + 1 &&   \
        PS_SAME_PROTO(x, y) \
)

/*
 * ID ethernet address.
 * It is used for all modelled interfaces and for identification of relevant
 * packets.
 */
#define ID_ETH_ADDR {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}

/*
 * A located abstract representative packet.
 */
class Packet
{
private:
    // ingress interface
    Interface *interface;
    // IP
    IPv4Address src_ip, dst_ip;
    // TCP
    uint16_t src_port, dst_port;
    uint32_t seq, ack;
    // packet state (L4/L7)
    uint8_t pkt_state;
    // L7 payload
    Payload *payload;

    friend class PacketHash;
    friend bool operator==(const Packet&, const Packet&);
    friend bool eq_except_intf(const Packet&, const Packet&);
    friend bool same_ips_ports(const Packet&, const Packet&);
    friend bool reversed_ips_ports(const Packet&, const Packet&);
    friend bool same_comm(const Packet&, const Packet&);

public:
    Packet();
    Packet(State *, const Policy *);
    Packet(Interface *);    // dummy packet, used unblocking listener thread
    Packet(const Packet&) = default;
    Packet(Packet&&) = default;

    Packet& operator=(const Packet&) = default;
    Packet& operator=(Packet&&) = default;

    std::string to_string() const;
    Interface *get_intf() const;
    IPv4Address get_src_ip() const;
    IPv4Address get_dst_ip() const;
    uint16_t get_src_port() const;
    uint16_t get_dst_port() const;
    uint32_t get_seq() const;
    uint32_t get_ack() const;
    uint8_t get_pkt_state() const;
    Payload *get_payload() const;
    void clear();
    bool empty() const;
    void set_intf(Interface *);
    void set_src_ip(const IPv4Address&);
    void set_dst_ip(const IPv4Address&);
    void set_src_port(uint16_t);
    void set_dst_port(uint16_t);
    void set_seq_no(uint32_t);
    void set_ack_no(uint32_t);
    void set_pkt_state(uint8_t);
};

bool operator==(const Packet&, const Packet&);
bool eq_except_intf(const Packet&, const Packet&);
bool same_ips_ports(const Packet&, const Packet&);
bool reversed_ips_ports(const Packet&, const Packet&);
bool same_comm(const Packet&, const Packet&);

struct PacketHash {
    size_t operator()(Packet *const&) const;
};

struct PacketEq {
    bool operator()(Packet *const&, Packet *const&) const;
};
