/*
 * Middlebox emulation instance.
 * An emulation instance consists of:
 *     - a network environment (e.g., network namespace) where the appliance
 * runs.
 *     - a listener thread for reading packets asynchronously.
 *     - a listener thread for monitoring packet drops asynchronously.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

#include "mb-env/mb-env.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
class Middlebox;

class Emulation {
private:
    MB_Env *env;            // environment
    Middlebox *emulated_mb; // currently emulated middlebox node
    NodePacketHistory *node_pkt_hist;
    std::unordered_map<int, uint32_t> seq_offsets;
    bool dropmon;

    std::thread *packet_listener;
    std::thread *drop_listener;
    std::atomic<bool> stop_listener;           // loop control flag for threads
    std::list<Packet> recv_pkts;               // received packets (race)
    std::unordered_set<size_t> recv_pkts_hash; // hashes of recv_pkts (race)
    std::atomic<uint64_t> drop_ts;             // kernel drop timestamp (race)
    std::mutex mtx; // lock for recv_pkts, recv_pkts_hash, and drop_ts
    std::condition_variable cv; // for reading recv_pkts

    void listen_packets();
    void listen_drops();
    void teardown();

private:
    friend class Config;
    friend class EmulationMgr;
    Emulation();

public:
    Emulation(const Emulation &) = delete;
    Emulation(Emulation &&) = delete;
    ~Emulation();
    Emulation &operator=(const Emulation &) = delete;
    Emulation &operator=(Emulation &&) = delete;

    Middlebox *get_mb() const;
    NodePacketHistory *get_node_pkt_hist() const;
    void reset_offsets();
    void set_offset(int conn, uint32_t offset);
    uint32_t get_offset(int conn) const;

    void init(Middlebox *); // initialize and start the emulation
    int rewind(NodePacketHistory *);
    std::list<Packet> send_pkt(const Packet &);
};
