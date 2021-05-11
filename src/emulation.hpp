/*
 * Middlebox emulation instance.
 * An emulation instance consists of:
 *     - a network environment (e.g., network namespace) where the appliance runs.
 *     - a listener thread for reading packets asynchronously.
 *     - a listener thread for monitoring packet drops asynchronously.
 */
#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "mb-env/mb-env.hpp"
#include "pkt-hist.hpp"
#include "packet.hpp"
class Middlebox;

class Emulation
{
private:
    MB_Env *env;            // environment
    Middlebox *emulated_mb; // currently emulated middlebox node
    NodePacketHistory *node_pkt_hist;
    bool dropmon;

    std::thread *packet_listener;
    std::thread *drop_listener;
    std::atomic<bool> stop_listener;    // loop control flag for threads
    std::vector<Packet> recv_pkts;      // received packets (race)
    std::atomic<uint64_t> drop_ts;      // kernel drop timestamp
    std::mutex mtx;                     // lock for accessing recv_pkts
    std::condition_variable cv;         // for reading recv_pkts

    void listen_packets();
    void listen_drops();
    void teardown();

private:
    friend class Config;
    friend class EmulationMgr;
    Emulation();

public:
    Emulation(const Emulation&) = delete;
    Emulation(Emulation&&) = delete;
    ~Emulation();
    Emulation& operator=(const Emulation&) = delete;
    Emulation& operator=(Emulation&&) = delete;

    Middlebox *get_mb() const;
    NodePacketHistory *get_node_pkt_hist() const;

    void init(Middlebox *);  // initialize and start the emulation
    int rewind(NodePacketHistory *);
    std::vector<Packet> send_pkt(const Packet&, bool rewinding = false);
};
