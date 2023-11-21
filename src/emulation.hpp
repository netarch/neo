/**
 * Middlebox emulation instance.
 * An emulation instance consists of:
 *     - an emulation driver that interacts with the actual NFVs.
 *     - a listener thread for reading packets asynchronously.
 *     - a listener thread for monitoring packet drops asynchronously.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "driver/driver.hpp"
#include "emu-pkt-key.hpp"
#include "injection-result.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
class Middlebox;

/**
 * @brief An emulation object represents a concrete emulated instance, which may
 * be used to emulate one or multiple middleboxes.
 *
 * This class handles all the low-level chores like interacting with drivers
 * (e.g., docker), spawning threads to listen for packets, and masking packet
 * seq offsets etc. Note that we intentionally separate the `node_pkt_hist`
 * methods from others because this class is unaware of and doesn't deal with
 * nph hashing and calculations.
 */
class Emulation {
private:
    Middlebox *_mb;                  // currently emulated middlebox node
    NodePacketHistory *_nph;         // current nph of the mb
    std::unique_ptr<Driver> _driver; // emulation driver
    std::unordered_map<EmuPktKey, uint32_t> _seq_offsets;
    std::unordered_map<EmuPktKey, uint16_t> _port_offsets;
    bool _dropmon;

    std::unique_ptr<std::thread> _recv_thread;
    std::unique_ptr<std::thread> _drop_thread;
    std::atomic<bool> _stop_recv;          // loop control flag
    std::atomic<bool> _stop_drop;          // loop control flag
    std::list<Packet> _recv_pkts;          // received packets (race)
    std::unordered_set<size_t> _pkts_hash; // hashes of _recv_pkts (race)
    std::atomic<uint64_t> _drop_ts;        // kernel drop timestamp (race)
    std::mutex _mtx; // lock for _recv_pkts, _pkts_hash, and _drop_ts
    std::condition_variable _cv; // for reading _recv_pkts

    void listen_packets();
    void listen_drops();
    void start_recv_thread();
    void stop_recv_thread();
    void start_drop_thread();
    void stop_drop_thread();

    void reset_offsets();
    void apply_offsets(Packet &) const;
    void update_offsets(std::list<Packet> &);

public:
    Emulation();
    Emulation(const Emulation &) = delete;
    Emulation(Emulation &&) = delete;
    ~Emulation();
    Emulation &operator=(const Emulation &) = delete;
    Emulation &operator=(Emulation &&) = delete;

    decltype(_mb) mb() const { return _mb; }
    decltype(_nph) node_pkt_hist() const { return _nph; }
    void node_pkt_hist(decltype(_nph) nph) { _nph = nph; }

    void teardown();
    void init(Middlebox *, bool log_pkts = true); // re-initialize the emulation
    int rewind(NodePacketHistory *);
    InjectionResult send_pkt(const Packet &);
};
