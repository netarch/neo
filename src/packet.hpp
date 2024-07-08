#pragma once

#include <string>

#include "droptrace.h"
#include "lib/ip.hpp"

class Driver;
class Interface;
class Model;
class Payload;

/**
 * ID ethernet address.
 * It is used for all modelled interfaces and for identification of relevant
 * packets.
 */
#define ID_ETH_ADDR {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}

/**
 * A located abstract representative packet.
 */
class Packet {
private:
    // ingress interface
    Interface *interface;
    // IP
    IPv4Address src_ip, dst_ip;
    // TCP
    uint16_t src_port, dst_port;
    uint32_t seq, ack;
    // packet state (L4/L7)
    uint16_t proto_state;
    // L7 payload
    Payload *payload;

    /**
     * Auxiliary variables, should not involved in PacketHash or PacketEq
     */
    int _conn;          // connection index
    bool _is_new;       // the packet belongs to a new connection
    bool _opposite_dir; // the packet is destined to the opposite direction
    bool _next_phase; // the packet starts the next phase within the connection

    friend class PacketHash;
    friend bool operator==(const Packet &, const Packet &);
    friend bool operator<(const Packet &, const Packet &);

public:
    Packet();
    Packet(const Model &);
    Packet(Interface *intf,
           IPv4Address src_ip,
           IPv4Address dst_ip,
           uint16_t src_port,
           uint16_t dst_port,
           uint32_t seq,
           uint32_t ack,
           uint16_t proto_state);
    Packet(const Packet &) = default;
    Packet(Packet &&) = default;

    Packet &operator=(const Packet &) = default;
    Packet &operator=(Packet &&) = default;

    std::string to_string() const;
    struct drop_data to_drop_data(Driver *driver = nullptr) const;
    Interface *get_intf() const;
    IPv4Address get_src_ip() const;
    IPv4Address get_dst_ip() const;
    uint16_t get_src_port() const;
    uint16_t get_dst_port() const;
    uint32_t get_seq() const;
    uint32_t get_ack() const;
    uint16_t get_proto_state() const;
    Payload *get_payload() const;
    size_t get_payload_size() const;
    int conn() const;
    bool is_new() const;
    bool opposite_dir() const;
    bool next_phase() const;
    void clear();
    bool empty() const;
    bool same_flow_as(const Packet &) const;
    bool same_header(const Packet &) const;
    void set_intf(Interface *);
    void set_src_ip(const IPv4Address &);
    void set_dst_ip(const IPv4Address &);
    void set_src_port(uint16_t);
    void set_dst_port(uint16_t);
    void set_seq(uint32_t);
    void set_ack(uint32_t);
    void set_proto_state(uint16_t);
    void set_payload(Payload *);
    void set_conn(int);
    void set_is_new(bool);
    void set_opposite_dir(bool);
    void set_next_phase(bool);
};

bool operator==(const Packet &, const Packet &);
bool operator<(const Packet &, const Packet &);

class PacketHash {
public:
    size_t operator()(const Packet &) const;
};

class PacketPtrHash {
public:
    size_t operator()(const Packet *const &) const;
};

class PacketPtrEq {
public:
    bool operator()(const Packet *const &, const Packet *const &) const;
};
