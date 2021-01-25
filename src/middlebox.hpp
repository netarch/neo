#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "node.hpp"
#include "mb-env/mb-env.hpp"
#include "mb-app/mb-app.hpp"
#include "pkt-hist.hpp"

class Middlebox : public Node
{
private:
    MB_Env *env;    // environment
    MB_App *app;    // appliance

    NodePacketHistory *node_pkt_hist;
    std::thread *listener;
    std::atomic<bool> listener_ended;
    std::vector<Packet> recv_pkts;  // received packets
    std::mutex mtx;                 // lock for accessing recv_pkts
    std::condition_variable cv;     // for reading recv_pkts

    void listen_packets();

private:
    friend class Config;
    Middlebox();

public:
    Middlebox(const Middlebox&) = delete;
    Middlebox(Middlebox&&) = delete;
    ~Middlebox() override;

    Middlebox& operator=(const Middlebox&) = delete;
    Middlebox& operator=(Middlebox&&) = delete;

    /*
     * Actually initialize and start the emulation.
     */
    void init() override;

    int rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::vector<Packet> send_pkt(const Packet&); // send one L7 packet

    /*
     * Return an empty set. We use emulations to get next hops for middleboxes.
     * The forwarding process will inject a concrete packet to the emulation
     * instance.
     */
    std::set<FIB_IPNH> get_ipnhs(
        const IPv4Address&,
        std::unordered_set<IPv4Address> *looked_up_ips = nullptr) override;
};
