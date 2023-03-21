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
    // Emulation driver type, currently only "docker" is supported
    std::string _driver;

    // Packet injection latency and drop timeout estimate
    std::chrono::microseconds _lat_avg, _lat_mdev, _timeout;
    int _mdev_scalar;

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

    const decltype(_driver) &driver() const { return _driver; }
    decltype(_lat_avg) lat_avg() const { return _lat_avg; }
    decltype(_lat_mdev) lat_mdev() const { return _lat_mdev; }
    decltype(_timeout) timeout() const { return _timeout; }
    decltype(_mdev_scalar) mdev_scalar() const { return _mdev_scalar; }
    decltype(_emulation) emulation() const { return _emulation; }
    const decltype(_ec_ip_prefixes) &ec_ip_prefixes() const {
        return _ec_ip_prefixes;
    }
    const decltype(_ec_ip_addrs) &ec_ip_addrs() const { return _ec_ip_addrs; }
    const decltype(_ec_ports) &ec_ports() const { return _ec_ports; }

    bool is_emulated() const override { return true; }
    void update_timeout();
    void adjust_latency_estimate_by_ntasks(int ntasks);

    int rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::list<Packet> send_pkt(const Packet &);

    /**
     * Return an empty set. We use emulations to get next hops for middleboxes,
     * instead of looking up from routing tables. The forwarding process will
     * inject a concrete packet to the emulation instance.
     */
    std::set<FIB_IPNH>
    get_ipnhs(const IPv4Address &,
              const RoutingTable *rib,
              std::unordered_set<IPv4Address> *looked_up_ips) override;
};
