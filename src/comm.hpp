#pragma once

#include <vector>

#include "lib/ip.hpp"
#include "node.hpp"
#include "eqclasses.hpp"
#include "eqclass.hpp"

enum proto {
    tcp,
    udp,
    icmp_echo
};

/*
 * An instance of Communication contains a set of independent ECs related to a
 * policy, which will be used for parallelizing the computation, and a set of
 * start nodes, which will be explored within one process for each independent
 * EC.
 */
class Communication
{
private:
    int                     protocol;
    IPRange<IPv4Address>    pkt_dst;
    bool                    owned_dst_only;
    std::vector<Node *>     start_nodes;

    EqClasses               ECs;
    EqClasses::iterator     ECs_itr;
    std::vector<uint16_t>   dst_ports;
    std::vector<uint16_t>::iterator dst_ports_itr;

    EqClass                 *initial_ec;    // per-process variable (assigned)
    uint16_t                src_port;       // per-process variable (fixed)
    uint16_t                dst_port;       // per-process variable (assigned)

    friend bool operator==(const Communication&, const Communication&);

private:
    friend class Config;
    Communication();

public:
    Communication(const Communication&) = delete;
    Communication(Communication&&) = default;

    int get_protocol() const;
    const std::vector<Node *>& get_start_nodes() const;
    std::string start_nodes_str() const;
    uint16_t get_src_port() const;
    uint16_t get_dst_port() const;
    void set_src_port(uint16_t);
    void set_dst_port(uint16_t);

    void compute_ecs(const EqClasses&, const EqClasses&, const std::set<uint16_t>&);
    void add_ec(const IPv4Address&);
    EqClass *find_ec(const IPv4Address&) const;
    size_t num_ecs() const;
    size_t num_start_nodes() const;

    bool set_initial_ec();
    EqClass *get_initial_ec() const;
};
