#pragma once

#include <chrono>
#include <list>
#include <string>

#include "emulation.hpp"
#include "mb-app/mb-app.hpp"
#include "node.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"

class Middlebox : public Node {
private:
    Emulation *emulation;
    std::string env;
    MB_App *app; // appliance
    std::chrono::microseconds latency_avg, latency_mdev, timeout;
    bool dropmon;
    int dev_scalar;

private:
    friend class Config;
    Middlebox();
    void reassemble_segments(std::list<Packet> &);

public:
    Middlebox(const Middlebox &) = delete;
    Middlebox(Middlebox &&) = delete;
    ~Middlebox() override;
    Middlebox &operator=(const Middlebox &) = delete;
    Middlebox &operator=(Middlebox &&) = delete;

    Emulation *get_emulation() const;
    std::string get_env() const;
    MB_App *get_app() const;
    std::chrono::microseconds get_timeout() const;
    bool dropmon_enabled() const;
    void update_timeout();
    void increase_latency_estimate_by_DOP(int DOP);

    int rewind(State *, NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::list<Packet> send_pkt(const Packet &);

    /*
     * Return an empty set. We use emulations to get next hops for middleboxes,
     * instead of looking up from routing tables. The forwarding process will
     * inject a concrete packet to the emulation instance.
     */
    std::set<FIB_IPNH>
    get_ipnhs(const IPv4Address &,
              const RoutingTable *rib,
              std::unordered_set<IPv4Address> *looked_up_ips) override;
};
