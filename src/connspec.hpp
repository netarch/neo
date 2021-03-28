#pragma once

#include <set>

#include "node.hpp"
#include "eqclass.hpp"
#include "conn.hpp"
#include "lib/ip.hpp"

/*
 * Connection specification:
 *  - Specify a set of independent connections.
 *  - Each connection within one ConnSpec will be populated to individual tasks.
 */
class ConnSpec
{
private:
    int                     protocol;
    std::set<Node *>        src_nodes;
    IPRange<IPv4Address>    dst_ip;
    std::set<EqClass *>     dst_ip_ecs;
    uint16_t                src_port;
    std::set<uint16_t>      dst_ports;
    bool                    owned_dst_only;

private:
    friend class Config;
    ConnSpec();

public:
    ConnSpec(ConnSpec&&) = default;
    void update_policy_ecs() const;
    std::set<Connection> compute_connections();
};
