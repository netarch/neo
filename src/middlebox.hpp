#pragma once

#include <chrono>
#include <list>
#include <set>
#include <string>

#include "emulation.hpp"
#include "lib/ip.hpp"
#include "node.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"

class Middlebox : public Node {
protected:
    // The actual emulation instance
    Emulation *_emulation;

    // Interesting IP and port values parsed from the config files
    std::set<IPNetwork<IPv4Address>> _ec_ip_prefixes;
    std::set<IPv4Address> _ec_ip_addrs;
    std::set<uint16_t> _ec_ports;

protected:
    friend class ConfigParser;
    Middlebox();

public:
    Middlebox(const Middlebox &) = delete;
    Middlebox(Middlebox &&) = delete;
    Middlebox &operator=(const Middlebox &) = delete;
    Middlebox &operator=(Middlebox &&) = delete;

    decltype(_emulation) emulation() const { return _emulation; }
    const decltype(_ec_ip_prefixes) &ec_ip_prefixes() const {
        return _ec_ip_prefixes;
    }
    const decltype(_ec_ip_addrs) &ec_ip_addrs() const { return _ec_ip_addrs; }
    const decltype(_ec_ports) &ec_ports() const { return _ec_ports; }

    void rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::list<Packet> send_pkt(const Packet &);

    bool is_emulated() const override { return true; }
    std::set<FIB_IPNH>
    get_ipnhs(const IPv4Address &,
              const RoutingTable *rib,
              std::unordered_set<IPv4Address> *looked_up_ips) override;
};
