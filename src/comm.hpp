#pragma once

#ifndef MAX_COMMS
#define MAX_COMMS 2
#endif

#include <vector>

#include "lib/ip.hpp"
#include "node.hpp"
#include "eqclasses.hpp"
#include "eqclass.hpp"
#include "network.hpp"
struct State;

enum proto {
    PR_HTTP = 0,
    PR_ICMP_ECHO = 1
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
    uint16_t                tx_port;    // source port of the communication
    uint16_t                rx_port;    // target port of the communication

    EqClasses               ECs;
    EqClasses::iterator     ECs_itr;

    EqClass                 *initial_ec;    // per-process variable

    friend bool operator==(const Communication&, const Communication&);

private:
    friend class Config;
    Communication(): initial_ec(nullptr) {}

public:
    Communication(const Communication&) = delete;
    Communication(Communication&&) = default;

    int get_protocol() const;
    const std::vector<Node *>& get_start_nodes() const;
    std::string start_nodes_str() const;
    uint16_t get_src_port(State *) const;
    uint16_t get_dst_port(State *) const;
    uint16_t get_tx_port() const;
    uint16_t get_rx_port() const;
    void set_tx_port(uint16_t);
    void set_rx_port(uint16_t);

    void compute_ecs(const EqClasses&, const EqClasses&);
    void add_ec(const IPv4Address&);
    EqClass *find_ec(const IPv4Address&) const;
    size_t num_ecs() const;
    size_t num_start_nodes() const;
    size_t num_comms() const;

    bool set_initial_ec();
    EqClass *get_initial_ec() const;
};

bool operator==(const Communication&, const Communication&);
