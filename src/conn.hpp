#pragma once

#include "node.hpp"
#include "eqclass.hpp"
struct State;

/*
 * Connection denoted by the initial 5-tuple flow
 */
class Connection
{
private:
    int         protocol;
    Node        *src_node;  // reflect the src IP addr
    EqClass     *dst_ip_ec;
    uint16_t    src_port;
    uint16_t    dst_port;

public:
    Connection(int protocol, Node *src_node, EqClass *dst_ip_ec,
               uint16_t src_port, uint16_t dst_port);
    Connection(const Connection&) = default;
    Connection(Connection&&) = default;

    void init(State *, size_t conn_idx, const Network&) const;
};
