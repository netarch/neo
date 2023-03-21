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
#include <mutex>
#include <thread>

#include "driver/driver.hpp"
#include "emu-pkt-key.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
class Middlebox;

class Emulation {
private:
    Driver *_driver; // emulation driver
    Middlebox *_mb;  // currently emulated middlebox node
    NodePacketHistory *_nph;
    std::unordered_map<EmuPktKey, uint32_t> _seq_offsets;
    std::unordered_map<EmuPktKey, uint16_t> _port_offsets;
    bool _dropmon;

    std::thread *_recv_thread;
    std::thread *_drop_thread;
    std::atomic<bool> _stop_threads;       // loop control flag for threads
    std::list<Packet> _recv_pkts;          // received packets (race)
    std::unordered_set<size_t> _pkts_hash; // hashes of _recv_pkts (race)
    std::atomic<uint64_t> _drop_ts;        // kernel drop timestamp (race)
    std::mutex _mtx; // lock for _recv_pkts, _pkts_hash, and _drop_ts
    std::condition_variable _cv; // for reading _recv_pkts

    void listen_packets();
    void listen_drops();
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
    void init(Middlebox *); // re-initialize and start the emulation
    int rewind(NodePacketHistory *);
    std::list<Packet> send_pkt(const Packet &);
};
