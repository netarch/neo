#include "emulation.hpp"

#include <cassert>
#include <csignal>
#include <libnet.h>

#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "driver/driver.hpp"
#include "lib/net.hpp"
#include "logger.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"
#include "protocols.hpp"
#include "stats.hpp"

using namespace std;

Emulation::Emulation()
    : _driver(nullptr), _mb(nullptr), _nph(nullptr), _dropmon(false),
      _recv_thread(nullptr), _drop_thread(nullptr), _stop_threads(false),
      _drop_ts(0) {}

Emulation::~Emulation() {
    // delete _recv_thread;
    // delete _drop_thread;
    // delete _driver;
    teardown();
}

void Emulation::teardown() {
    if (_recv_thread) {
        _stop_threads = true;
        pthread_kill(_recv_thread->native_handle(), SIGUSR1);
        if (_recv_thread->joinable()) {
            _recv_thread->join();
        }
    }
    if (_drop_thread) {
        _stop_threads = true;
        pthread_kill(_drop_thread->native_handle(), SIGUSR1);
        if (_drop_thread->joinable()) {
            _drop_thread->join();
        }
    }
    delete _recv_thread;
    delete _drop_thread;
    delete _driver;
    _driver = nullptr;
    _mb = nullptr;
    _nph = nullptr;
    _recv_thread = nullptr;
    _drop_thread = nullptr;
    _stop_threads = false;
}

void Emulation::listen_packets() {
    list<Packet> pkts;
    PacketHash hasher;

    while (!_stop_threads) {
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

    while (!_stop_threads) {
        // ts = DropMon::get().is_dropped();

        if (ts) {
            unique_lock<mutex> lck(_mtx);
            _recv_pkts.clear();
            _pkts_hash.clear();
            _drop_ts = ts;
            _cv.notify_all();
        }
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

void Emulation::init(Middlebox *mb) {
    if (_mb != mb) {
        this->teardown();

        if (mb->driver() == "docker") {
            _driver = new Docker(dynamic_cast<DockerNode *>(mb));
        } else {
            logger.error("Unknown driver: " + mb->driver());
        }
        _driver->init();
        unique_lock<mutex> lck(_mtx);
        _recv_pkts.clear();
        _pkts_hash.clear();

        // spawn the packet_listener thread (block all signals but SIGUSR1)
        sigset_t mask, old_mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGHUP);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
        _recv_thread = new thread(&Emulation::listen_packets, this);
        pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);

        // spawn the drop_listener thread (block all signals but SIGUSR1)
        // if (mb->dropmon_enabled()) {
        //     _dropmon = true;
        //     pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
        //     drop_listener = new thread(&Emulation::listen_drops, this);
        //     pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
        // }

        _mb = mb;
    } else {
        _driver->init();
        unique_lock<mutex> lck(_mtx);
        _recv_pkts.clear();
        _pkts_hash.clear();
    }

    this->reset_offsets();
}

int Emulation::rewind(NodePacketHistory *nph) {
    if (_nph == nph) {
        logger.info(_mb->get_name() + " up to date, no need to rewind");
        return -1;
    }

    int rewind_injections = 0;
    logger.info("Rewinding " + _mb->get_name() + "...");

    // reset middlebox state
    if (!nph || !nph->contains(_nph)) {
        unique_lock<mutex> lck(_mtx);
        _recv_pkts.clear();
        _pkts_hash.clear();
        _driver->reset();
        this->reset_offsets();
        logger.info("Reset " + _mb->get_name());
    }

    // replay history
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

list<Packet> Emulation::send_pkt(const Packet &pkt) {
    // Apply offsets
    Packet new_pkt(pkt);
    this->apply_offsets(new_pkt);

    unique_lock<mutex> lck(_mtx);
    size_t num_pkts = _recv_pkts.size();
    // DropMon::get().start_listening_for(new_pkt);

    Stats::set_pkt_lat_t1();
    _driver->inject_packet(new_pkt);

    // Read packets iteratively until no new packets are read within one
    // complete timeout period.
    do {
        num_pkts = _recv_pkts.size();

        if (_dropmon) { // use drop monitor
            // // TODO: Think about how to incorporate dropmon with timeouts
            // _drop_ts = 0;
            // chrono::microseconds timeout(5000);
            // cv_status status = _cv.wait_for(lck, timeout);
            // Stats::set_pkt_latency(timeout, _drop_ts);

            // if (status == cv_status::timeout && _recv_pkts.empty() &&
            //     _drop_ts == 0) {
            //     logger.error("Drop monitor timed out!");
            // }
        } else { // use timeout (new injection)
            _cv.wait_for(lck, _mb->timeout());
            Stats::set_pkt_latency(_mb->timeout());
        }
    } while (_recv_pkts.size() > num_pkts);

    // DropMon::get().stop_listening();

    // Move and reset the received packets
    list<Packet> pkts(std::move(_recv_pkts));
    _recv_pkts.clear();
    lck.unlock();

    Net::get().reassemble_segments(pkts);
    this->update_offsets(pkts);
    return pkts;
}
