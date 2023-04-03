#include "emulation.hpp"

#include <cassert>
#include <csignal>
#include <libnet.h>
#include <typeinfo>

#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "driver/driver.hpp"
#include "dropdetection.hpp"
#include "dropmon.hpp"
#include "droptimeout.hpp"
#include "droptrace.hpp"
#include "lib/net.hpp"
#include "logger.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"
#include "protocols.hpp"
#include "stats.hpp"

using namespace std;

Emulation::Emulation()
    : _mb(nullptr), _nph(nullptr), _dropmon(false), _stop_recv(false),
      _stop_drop(false), _drop_ts(0) {}

Emulation::~Emulation() {
    teardown();
}

void Emulation::teardown() {
    stop_recv_thread();
    stop_drop_thread();

    _mb = nullptr;
    _nph = nullptr;
    _driver.reset();
    _seq_offsets.clear();
    _port_offsets.clear();
    _dropmon = false;

    lock_guard<mutex> lck(_mtx);
    this->_recv_pkts.clear();
    this->_pkts_hash.clear();
    this->_drop_ts = 0;
}

void Emulation::listen_packets() {
    list<Packet> pkts;
    PacketHash hasher;

    while (!_stop_recv) {
        // read the output packets (it will block if there is no packet)
        pkts = _driver->read_packets();

        if (!pkts.empty()) {
            unique_lock<mutex> lck(_mtx);

            // Remove the retransmitted packets
            auto p = pkts.begin();
            while (p != pkts.end()) {
                size_t hash_value = hasher(&*p);
                if (_pkts_hash.count(hash_value) > 0) {
                    pkts.erase(p++);
                } else {
                    _pkts_hash.insert(hash_value);
                    p++;
                }
            }

            _recv_pkts.splice(_recv_pkts.end(), pkts);
            _cv.notify_all();
        }
    }
}

void Emulation::listen_drops() {
    uint64_t ts = 0;

    while (!_stop_drop) {
        if (drop) {
            ts = drop->get_drop_ts();
        } else {
            break;
        }

        if (ts) {
            unique_lock<mutex> lck(_mtx);
            _recv_pkts.clear();
            _pkts_hash.clear();
            _drop_ts = ts;
            _cv.notify_all();
        }
    }
}

void Emulation::start_recv_thread() {
    // Set up the recv thread (block all signals but SIGUSR1)
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
    _recv_thread = make_unique<thread>(&Emulation::listen_packets, this);
    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
}

void Emulation::stop_recv_thread() {
    if (_recv_thread) {
        _stop_recv = true;
        pthread_kill(_recv_thread->native_handle(), SIGUSR1);
        if (_recv_thread->joinable()) {
            _recv_thread->join();
        }
        _recv_thread.reset();
        _stop_recv = false;
    }
}

void Emulation::start_drop_thread() {
    // Set up the drop thread (block all signals but SIGUSR1)
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
    _drop_thread = make_unique<thread>(&Emulation::listen_drops, this);
    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
}

void Emulation::stop_drop_thread() {
    if (_drop_thread) {
        _stop_drop = true;
        pthread_kill(_drop_thread->native_handle(), SIGUSR1);
        if (_drop_thread->joinable()) {
            _drop_thread->join();
        }
        _drop_thread.reset();
        _stop_drop = false;
    }
}

void Emulation::reset_offsets() {
    this->_seq_offsets.clear();
    this->_port_offsets.clear();
}

void Emulation::apply_offsets(Packet &pkt) const {
    // Skip any non-TCP packets
    if (!PS_IS_TCP(pkt.get_proto_state())) {
        return;
    }

    // Skip if this middlebox is not an endpoint
    if (!this->_mb->has_ip(pkt.get_dst_ip())) {
        return;
    }

    EmuPktKey key(pkt.get_src_ip(), pkt.get_src_port());

    // Apply seq offset to the ack number
    uint32_t seq_offset = 0;
    auto i = this->_seq_offsets.find(key);
    if (i != this->_seq_offsets.end()) {
        seq_offset = i->second;
    }
    pkt.set_ack(pkt.get_ack() + seq_offset);

    // Apply port offset to the dst port number
    uint16_t port_offset = 0;
    auto j = this->_port_offsets.find(key);
    if (j != this->_port_offsets.end()) {
        port_offset = j->second;
    }
    pkt.set_dst_port(pkt.get_dst_port() + port_offset);
}

void Emulation::update_offsets(list<Packet> &pkts) {
    for (auto &pkt : pkts) {
        // Skip any non-TCP packets
        if (!PS_IS_TCP(pkt.get_proto_state())) {
            return;
        }

        // Skip if this middlebox is not an endpoint
        if (!this->_mb->has_ip(pkt.get_src_ip())) {
            return;
        }

        EmuPktKey key(pkt.get_dst_ip(), pkt.get_dst_port());
        uint16_t flags = pkt.get_proto_state() & (~0x800U);

        // Update seq offset
        if (flags == TH_SYN || flags == (TH_SYN | TH_ACK)) {
            this->_seq_offsets[key] = pkt.get_seq();
            pkt.set_seq(0);
        } else {
            uint32_t offset = 0;
            auto i = this->_seq_offsets.find(key);
            if (i != this->_seq_offsets.end()) {
                offset = i->second;
            }
            pkt.set_seq(pkt.get_seq() - offset);
        }

        // Update (src) port offset
        if (flags == TH_SYN) {
            if (pkt.get_src_port() >= 1024) {
                this->_port_offsets[key] = pkt.get_src_port() - DYNAMIC_PORT;
                pkt.set_src_port(DYNAMIC_PORT);
            }
        } else {
            uint16_t offset = 0;
            auto i = this->_port_offsets.find(key);
            if (i != this->_port_offsets.end()) {
                offset = i->second;
            }
            pkt.set_src_port(pkt.get_src_port() - offset);
        }
    }
}

/**
 * It initializes the emulation by creating a driver object and launching the
 * emulation.
 *
 * @param mb the middlebox object
 * @param log_pkts whether to log packets
 */
void Emulation::init(Middlebox *mb, bool log_pkts) {
    // Reset everything as if it's just constructed.
    this->teardown();

    if (typeid(*mb) == typeid(DockerNode)) {
        _driver = make_unique<Docker>(dynamic_cast<DockerNode *>(mb), log_pkts);
    } else {
        logger.error("Unsupported middlebox type");
    }

    _mb = mb;
    _driver->init(); // Launch the emulation
    _driver->pause();

    // drop monitor
    // if (mb->dropmon_enabled()) {
    //     _dropmon = true;
    // }
}

/**
 * It resets the emulation state and re-injects all packets that have been sent
 * since the last checkpoint.
 *
 * Notice that this function does NOT update the node packet history (nph).
 *
 * @param nph the node packet history to rewind to
 *
 * @return The number of rewind injections. -1 means the emulation is already up
 * to date and no resetting occurred.
 */
int Emulation::rewind(NodePacketHistory *nph) {
    if (_nph == nph) {
        logger.info(_mb->get_name() + " up to date, no need to rewind");
        return -1;
    }

    logger.info("Rewinding " + _mb->get_name() + "...");

    // Reset the emulation state
    if (!nph || !nph->contains(_nph)) {
        unique_lock<mutex> lck(_mtx);
        reset_offsets();
        _driver->reset();
        _recv_pkts.clear();
        _pkts_hash.clear();
        logger.info("Reset " + _mb->get_name());
    }

    // Replay history
    int rewind_injections = 0;

    if (nph) {
        list<Packet *> pkts = nph->get_packets_since(_nph);
        rewind_injections = pkts.size();
        for (Packet *packet : pkts) {
            send_pkt(*packet);
        }
    }

    logger.info(to_string(rewind_injections) + " rewind injections");
    return rewind_injections;
}

/**
 * It sends a packet, waits for a timeout, and returns the received packets.
 * Notice that this function does NOT update the node packet history (nph).
 *
 * @param send_pkt The packet to send.
 *
 * @return A list of received packets.
 */
list<Packet> Emulation::send_pkt(const Packet &send_pkt) {
    // Prepare send packet, apply offsets
    Packet pkt(send_pkt);
    this->apply_offsets(pkt);

    // Reset packet receive buffer and drop ts
    unique_lock<mutex> lck(_mtx);
    _recv_pkts.clear();
    _pkts_hash.clear();
    _drop_ts = 0;

    // Set up the recv and drop threads
    start_recv_thread();
    if (drop) {
        drop->start_listening_for(pkt, _driver.get());
        start_drop_thread();
    }

    // Send the concrete packet
    _driver->unpause();
    _STATS_START(Stats::Op::PKT_LAT);
    _STATS_START(Stats::Op::DROP_LAT);
    _driver->inject_packet(pkt);

    // Receive packets iteratively until:
    // (1) the injected packet was detected dropped, or
    // (2) no new packets are read within one complete timeout period.
    size_t num_pkts = 0;
    bool first_recv = true;

    do {
        num_pkts = _recv_pkts.size();
        cv_status status = _cv.wait_for(lck, DropTimeout::get().timeout());

        if (_drop_ts != 0) { // Packet drop detected
            assert(_recv_pkts.empty());
            break;
        }

        if (first_recv && _recv_pkts.size() > num_pkts) {
            // PKT_LAT records the time for the first response packet
            _STATS_STOP(Stats::Op::PKT_LAT);
            first_recv = false;
        }
    } while (_recv_pkts.size() > num_pkts);

    // When _drop_ts != 0, the packet was certainly dropped.
    // But when _drop_ts == 0 and _recv_pkts.empty():
    // (1) if drop != null, then the packet was silently accepted, or
    // (2) if drop == null, then we don't know for sure, the packet may be
    // dropped or accepted silently.
    if (_drop_ts != 0) {
        _STATS_STOP(Stats::Op::DROP_LAT); // Packet dropped
    } else if (_recv_pkts.empty()) {
        if (drop) {
            _STATS_STOP(Stats::Op::PKT_LAT); // Packet accepted in silence
        } else {
            _STATS_STOP(Stats::Op::DROP_LAT); // May be dropped or accepted
        }
    }

    _driver->pause();

    // Stop the recv and drop threads
    lck.unlock();
    stop_recv_thread();
    if (drop) {
        stop_drop_thread();
        drop->stop_listening();
    }
    lck.lock();

    // Move and reset the received packets
    list<Packet> pkts(std::move(_recv_pkts));
    _recv_pkts.clear();
    _pkts_hash.clear();

    // Process the received packets
    Net::get().reassemble_segments(pkts);
    this->update_offsets(pkts);

    // Update drop timeout estimates
    if (!pkts.empty()) {
        DropTimeout::get().update_timeout();
    }

    return pkts;
}
