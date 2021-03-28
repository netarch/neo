#pragma once

#include <string>

#include "lib/ip.hpp"
class Interface;
class Payload;
struct State;

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
    uint8_t proto_state;
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
    uint8_t get_proto_state() const;
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
    void set_proto_state(uint8_t);
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
