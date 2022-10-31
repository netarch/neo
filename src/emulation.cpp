#include "emulation.hpp"

#include <cassert>
#include <csignal>
#include <libnet.h>

#include "dropmon.hpp"
#include "lib/logger.hpp"
#include "lib/net.hpp"
#include "mb-env/netns.hpp"
#include "middlebox.hpp"
#include "model-access.hpp"
#include "protocols.hpp"
#include "stats.hpp"

Emulation::Emulation()
    : env(nullptr), emulated_mb(nullptr), node_pkt_hist(nullptr),
      dropmon(false), packet_listener(nullptr), drop_listener(nullptr),
      stop_listener(false) {}

Emulation::~Emulation() {
    delete packet_listener;
    delete drop_listener;
    delete env;
}

void Emulation::listen_packets() {
    std::list<Packet> pkts;
    PacketHash hasher;

    while (!stop_listener) {
        // read the output packets (it will block if there is no packet)
        pkts = env->read_packets();

        if (!pkts.empty()) {
            std::unique_lock<std::mutex> lck(mtx);

            // Remove the retransmitted packets
            auto p = pkts.begin();
            while (p != pkts.end()) {
                size_t hash_value = hasher(&*p);
                if (recv_pkts_hash.count(hash_value) > 0) {
                    pkts.erase(p++);
                } else {
                    recv_pkts_hash.insert(hash_value);
                    p++;
                }
            }

            recv_pkts.splice(recv_pkts.end(), pkts);
            cv.notify_all();
        }
    }
}

void Emulation::listen_drops() {
    uint64_t ts;

    while (!stop_listener) {
        ts = DropMon::get().is_dropped();

        if (ts) {
            std::unique_lock<std::mutex> lck(mtx);
            recv_pkts.clear();
            recv_pkts_hash.clear();
            drop_ts = ts;
            cv.notify_all();
        }
    }
}

void Emulation::teardown() {
    if (packet_listener) {
        stop_listener = true;
        pthread_kill(packet_listener->native_handle(), SIGUSR1);
        if (packet_listener->joinable()) {
            packet_listener->join();
        }
    }
    if (drop_listener) {
        stop_listener = true;
        pthread_kill(drop_listener->native_handle(), SIGUSR1);
        if (drop_listener->joinable()) {
            drop_listener->join();
        }
    }
    delete packet_listener;
    delete drop_listener;
    delete env;
    env = nullptr;
    emulated_mb = nullptr;
    node_pkt_hist = nullptr;
    packet_listener = nullptr;
    drop_listener = nullptr;
    stop_listener = false;
}

void Emulation::reset_offsets() {
    this->seq_offsets.clear();
    this->port_offsets.clear();
}

void Emulation::apply_offsets(Packet &pkt) const {
    // Skip any non-TCP packets
    if (!PS_IS_TCP(pkt.get_proto_state())) {
        return;
    }

    // Skip if this middlebox is not an endpoint
    if (!this->emulated_mb->has_ip(pkt.get_dst_ip())) {
        return;
    }

    EmuPktKey key(pkt.get_src_ip(), pkt.get_src_port());

    // Apply seq offset to the ack number
    uint32_t seq_offset = 0;
    auto i = this->seq_offsets.find(key);
    if (i != this->seq_offsets.end()) {
        seq_offset = i->second;
    }
    pkt.set_ack(pkt.get_ack() + seq_offset);

    // Apply port offset to the dst port number
    uint16_t port_offset = 0;
    auto j = this->port_offsets.find(key);
    if (j != this->port_offsets.end()) {
        port_offset = j->second;
    }
    pkt.set_dst_port(pkt.get_dst_port() + port_offset);
}

void Emulation::update_offsets(std::list<Packet> &pkts) {
    for (auto &pkt : pkts) {
        this->update_offsets(pkt);
    }
}

void Emulation::update_offsets(Packet &pkt) {
    // Skip any non-TCP packets
    if (!PS_IS_TCP(pkt.get_proto_state())) {
        return;
    }

    // Skip if this middlebox is not an endpoint
    if (!this->emulated_mb->has_ip(pkt.get_src_ip())) {
        return;
    }

    EmuPktKey key(pkt.get_dst_ip(), pkt.get_dst_port());
    uint16_t flags = pkt.get_proto_state() & (~0x800U);

    // Update seq offset
    if (flags == TH_SYN || flags == (TH_SYN | TH_ACK)) {
        this->seq_offsets[key] = pkt.get_seq();
        pkt.set_seq(0);
    } else {
        uint32_t offset = 0;
        auto i = this->seq_offsets.find(key);
        if (i != this->seq_offsets.end()) {
            offset = i->second;
        }
        pkt.set_seq(pkt.get_seq() - offset);
    }

    // Update (src) port offset
    if (flags == TH_SYN) {
        if (pkt.get_src_port() >= 1024) {
            this->port_offsets[key] = pkt.get_src_port() - DYNAMIC_PORT;
            pkt.set_src_port(DYNAMIC_PORT);
        }
    } else {
        uint16_t offset = 0;
        auto i = this->port_offsets.find(key);
        if (i != this->port_offsets.end()) {
            offset = i->second;
        }
        pkt.set_src_port(pkt.get_src_port() - offset);
    }
}

Middlebox *Emulation::get_mb() const {
    return emulated_mb;
}

NodePacketHistory *Emulation::get_node_pkt_hist() const {
    return node_pkt_hist;
}

void Emulation::init(Middlebox *mb) {
    if (emulated_mb != mb) {
        this->teardown();

        if (mb->get_env() == "netns") {
            env = new NetNS();
        } else {
            Logger::error("Unknown environment: " + mb->get_env());
        }
        env->init(*mb);
        env->run(mb_app_init, mb->get_app());
        std::unique_lock<std::mutex> lck(mtx);
        recv_pkts.clear();
        recv_pkts_hash.clear();

        // spawn the packet_listener thread (block all signals but SIGUSR1)
        sigset_t mask, old_mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigaddset(&mask, SIGHUP);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
        packet_listener = new std::thread(&Emulation::listen_packets, this);
        pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);

        // spawn the drop_listener thread (block all signals but SIGUSR1)
        if (mb->dropmon_enabled()) {
            dropmon = true;
            pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
            drop_listener = new std::thread(&Emulation::listen_drops, this);
            pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
        }

        emulated_mb = mb;
    } else {
        env->run(mb_app_init, mb->get_app());
        std::unique_lock<std::mutex> lck(mtx);
        recv_pkts.clear();
        recv_pkts_hash.clear();
    }

    this->reset_offsets();
}

int Emulation::rewind(NodePacketHistory *nph) {
    const std::string node_name = emulated_mb->get_name();

    if (node_pkt_hist == nph) {
        Logger::info(node_name + " up to date, no need to rewind");
        return -1;
    }

    int rewind_injections = 0;
    Logger::info("Rewinding " + node_name + "...");

    // reset middlebox state
    if (!nph || !nph->contains(node_pkt_hist)) {
        std::unique_lock<std::mutex> lck(mtx);
        recv_pkts.clear();
        recv_pkts_hash.clear();
        env->run(mb_app_reset, emulated_mb->get_app());
        this->reset_offsets();
        Logger::info("Reset " + node_name);
    }

    // replay history
    if (nph) {
        std::list<Packet *> pkts = nph->get_packets_since(node_pkt_hist);
        rewind_injections = pkts.size();
        for (Packet *packet : pkts) {
            send_pkt(*packet);
        }
    }

    Logger::info(std::to_string(rewind_injections) + " rewind injections");
    return rewind_injections;
}

std::list<Packet> Emulation::send_pkt(const Packet &pkt) {
    // Apply offsets
    Packet new_pkt(pkt);
    this->apply_offsets(new_pkt);

    std::unique_lock<std::mutex> lck(mtx);
    size_t num_pkts = recv_pkts.size();
    DropMon::get().start_listening_for(new_pkt);

    Stats::set_pkt_lat_t1();
    env->inject_packet(new_pkt);

    // Read packets iteratively until no new packets are read within one
    // complete timeout period.
    do {
        num_pkts = recv_pkts.size();

        if (dropmon) { // use drop monitor
            // TODO: Think about how to incorporate dropmon with timeouts
            drop_ts = 0;
            std::chrono::microseconds timeout(5000);
            std::cv_status status = cv.wait_for(lck, timeout);
            Stats::set_pkt_latency(timeout, drop_ts);

            if (status == std::cv_status::timeout && recv_pkts.empty() &&
                drop_ts == 0) {
                Logger::error("Drop monitor timed out!");
            }
        } else { // use timeout (new injection)
            cv.wait_for(lck, emulated_mb->get_timeout());
            Stats::set_pkt_latency(emulated_mb->get_timeout());
        }
    } while (recv_pkts.size() > num_pkts);

    DropMon::get().stop_listening();

    // Move and reset the received packets
    std::list<Packet> pkts(std::move(recv_pkts));
    recv_pkts.clear();
    lck.unlock();

    Net::get().reassemble_segments(pkts);
    this->update_offsets(pkts);
    return pkts;
}
