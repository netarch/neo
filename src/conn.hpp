#pragma once

#include <string>

#include "eqclass.hpp"
#include "node.hpp"
class Packet;
class Network;

/*
 * Connection denoted by the initial 5-tuple flow
 */
class Connection {
private:
    int protocol;
    Node *src_node; // reflect the src IP addr
    EqClass *dst_ip_ec;
    uint16_t src_port;
    uint16_t dst_port;

    /* used by real packets (in-band connection creation) */
    IPv4Address src_ip;
    uint32_t seq;
    uint32_t ack;

    friend bool operator<(const Connection &, const Connection &);

public:
    Connection(int protocol,
               Node *src_node,
               EqClass *dst_ip_ec,
               uint16_t src_port,
               uint16_t dst_port);
    Connection(const Packet &, Node *src_node);
    Connection(const Connection &) = default;
    Connection(Connection &&) = default;

    std::string to_string() const;
    void init(size_t conn_idx, const Network &) const;
};

bool operator<(const Connection &, const Connection &);
