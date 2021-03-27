#pragma once

#include <string>

#include "lib/ip.hpp"
class Interface;
class Payload;
struct State;

/*
 * Packet state
 */
#define PS_TCP_INIT_1       0   // TCP 3-way handshake SYN
#define PS_TCP_INIT_2       1   // TCP 3-way handshake SYN/ACK
#define PS_TCP_INIT_3       2   // TCP 3-way handshake ACK
#define PS_TCP_L7_REQ       3   // TCP L7 request
#define PS_TCP_L7_REQ_A     4   // TCP L7 request ACK
#define PS_TCP_L7_REP       5   // TCP L7 reply
#define PS_TCP_L7_REP_A     6   // TCP L7 reply ACK
#define PS_TCP_TERM_1       7   // TCP termination FIN/ACK
#define PS_TCP_TERM_2       8   // TCP termination FIN/ACK
#define PS_TCP_TERM_3       9   // TCP termination ACK
#define PS_UDP_REQ         10   // UDP request
#define PS_UDP_REP         11   // UDP reply
#define PS_ICMP_ECHO_REQ   12   // ICMP echo request
#define PS_ICMP_ECHO_REP   13   // ICMP echo reply

#define PS_IS_REQUEST_DIR(x) (    \
        (x) == PS_TCP_INIT_1   || \
        (x) == PS_TCP_INIT_3   || \
        (x) == PS_TCP_L7_REQ   || \
        (x) == PS_TCP_L7_REP_A || \
        (x) == PS_TCP_TERM_2   || \
        (x) == PS_UDP_REQ      || \
        (x) == PS_ICMP_ECHO_REQ   \
)
#define PS_IS_REPLY_DIR(x) (      \
        (x) == PS_TCP_INIT_2   || \
        (x) == PS_TCP_L7_REQ_A || \
        (x) == PS_TCP_L7_REP   || \
        (x) == PS_TCP_TERM_1   || \
        (x) == PS_TCP_TERM_3   || \
        (x) == PS_UDP_REP      || \
        (x) == PS_ICMP_ECHO_REP   \
)
#define PS_IS_REQUEST(x) ( \
        (x) == PS_TCP_L7_REQ || \
        (x) == PS_UDP_REQ    || \
        (x) == PS_ICMP_ECHO_REQ \
)
#define PS_IS_REPLY(x) ( \
        (x) == PS_TCP_L7_REP || \
        (x) == PS_UDP_REP    || \
        (x) == PS_ICMP_ECHO_REP \
)
#define PS_IS_FIRST(x) (        \
        (x) == PS_TCP_INIT_1 || \
        (x) == PS_UDP_REQ    || \
        (x) == PS_ICMP_ECHO_REQ \
)
#define PS_IS_LAST(x) (        \
        (x) == PS_TCP_TERM_3 || \
        (x) == PS_UDP_REP    || \
        (x) == PS_ICMP_ECHO_REP \
)
#define PS_IS_TCP(x)       ((x) >= PS_TCP_INIT_1 && (x) <= PS_TCP_TERM_3)
#define PS_IS_UDP(x)       ((x) >= PS_UDP_REQ && (x) <= PS_UDP_REP)
#define PS_IS_ICMP_ECHO(x) ((x) >= PS_ICMP_ECHO_REQ && (x) <= PS_ICMP_ECHO_REP)
#define PS_HAS_SYN(x)      ((x) == PS_TCP_INIT_1 || (x) == PS_TCP_INIT_2)
#define PS_HAS_FIN(x)      ((x) == PS_TCP_TERM_1 || (x) == PS_TCP_TERM_2)
#define PS_SAME_PROTO(x, y) (                       \
        (PS_IS_TCP(x) && PS_IS_TCP(y)) ||           \
        (PS_IS_UDP(x) && PS_IS_UDP(y)) ||           \
        (PS_IS_ICMP_ECHO(x) && PS_IS_ICMP_ECHO(y))  \
)
#define PS_IS_NEXT(x, y) (  \
        (x) == (y) + 1 &&   \
        PS_SAME_PROTO(x, y) \
)
#define PS_LAST_PKT_STATE(x) ( \
        PS_IS_TCP(x) ? PS_TCP_TERM_3 : \
        (PS_IS_UDP(x) ? PS_UDP_REP : PS_ICMP_ECHO_REP) \
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

public:
    Packet();
    Packet(State *);
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

class PacketHash
{
public:
    size_t operator()(Packet *const&) const;
};

class PacketEq
{
public:
    bool operator()(Packet *const&, Packet *const&) const;
};
