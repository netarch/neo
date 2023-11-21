#pragma once

#include <chrono>
#include <list>
#include <set>
#include <string>
#include <sys/types.h>
#include <utility>

#include "emulation.hpp"
#include "lib/ip.hpp"
#include "node.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"

class Middlebox : public Node {
protected:
    useconds_t _start_delay = 0;
    useconds_t _reset_delay = 0;
    useconds_t _replay_delay = 0;
    // Number of the packets to send per injection.
    int _packets_per_injection = 0;
    // The actual emulation instance
    Emulation *_emulation = nullptr;

    // Interesting IP and port values parsed from the config files
    std::set<IPNetwork<IPv4Address>> _ec_ip_prefixes;
    std::set<IPv4Address> _ec_ip_addrs;
    std::set<uint16_t> _ec_ports;

protected:
    friend class ConfigParser;
    Middlebox() = default;

public:
    Middlebox(const Middlebox &) = delete;
    Middlebox(Middlebox &&) = delete;
    Middlebox &operator=(const Middlebox &) = delete;
    Middlebox &operator=(Middlebox &&) = delete;

    decltype(_start_delay) start_delay() const { return _start_delay; }
    decltype(_reset_delay) reset_delay() const { return _reset_delay; }
    decltype(_replay_delay) replay_delay() const { return _replay_delay; }
    decltype(_packets_per_injection) packets_per_injection() const {
        return _packets_per_injection;
    }
    decltype(_emulation) emulation() const { return _emulation; }
    const decltype(_ec_ip_prefixes) &ec_ip_prefixes() const {
        return _ec_ip_prefixes;
    }
    const decltype(_ec_ip_addrs) &ec_ip_addrs() const { return _ec_ip_addrs; }
    const decltype(_ec_ports) &ec_ports() const { return _ec_ports; }

    void rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::pair<std::list<Packet>, bool> send_pkt(const Packet &);

    bool is_emulated() const override { return true; }
    std::set<FIB_IPNH>
    get_ipnhs(const IPv4Address &,
              const RoutingTable *rib,
              std::unordered_set<IPv4Address> *looked_up_ips) override;
};
