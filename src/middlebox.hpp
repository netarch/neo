#pragma once

#include <string>
#include <chrono>
#include <vector>

#include "node.hpp"
#include "emulation.hpp"
#include "mb-app/mb-app.hpp"
#include "pkt-hist.hpp"
#include "packet.hpp"

class Middlebox : public Node
{
private:
    Emulation *emulation;
    std::string env;
    MB_App *app;    // appliance
    std::chrono::microseconds timeout;

private:
    friend class Config;
    Middlebox();

public:
    Middlebox(const Middlebox&) = delete;
    Middlebox(Middlebox&&) = delete;
    ~Middlebox() override;
    Middlebox& operator=(const Middlebox&) = delete;
    Middlebox& operator=(Middlebox&&) = delete;

    std::string get_env() const;
    MB_App *get_app() const;
    std::chrono::microseconds get_timeout() const;

    int rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::vector<Packet> send_pkt(const Packet&);

    /*
     * Return an empty set. We use emulations to get next hops for middleboxes,
     * instead of looking up from routing tables. The forwarding process will
     * inject a concrete packet to the emulation instance.
     */
    std::set<FIB_IPNH> get_ipnhs(
        const IPv4Address&,
        const RoutingTable *rib,
        std::unordered_set<IPv4Address> *looked_up_ips) override;
};
