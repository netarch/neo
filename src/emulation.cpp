#include "emulation.hpp"

#include <csignal>

#include "dropmon.hpp"
#include "lib/logger.hpp"
#include "mb-env/netns.hpp"
#include "middlebox.hpp"
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
    std::vector<Packet> pkts;

    while (!stop_listener) {
        // read the output packets (it will block if there is no packet)
        pkts = env->read_packets();

        if (!pkts.empty()) {
            std::unique_lock<std::mutex> lck(mtx);
            // concatenate, NOT REPLACE
            recv_pkts.insert(recv_pkts.end(), pkts.begin(), pkts.end());

            // make sure that we always wait for full timeout (an injected
            // packet may get multiple responses)
            // cv.notify_all();
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
    }
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
        env->run(mb_app_reset, emulated_mb->get_app());
        Logger::info("Reset " + node_name);
    }

    // replay history
    if (nph) {
        std::list<Packet *> pkts = nph->get_packets_since(node_pkt_hist);
        rewind_injections = pkts.size();
        for (Packet *packet : pkts) {
            send_pkt(*packet, /* rewinding */ true);
        }
    }

    Logger::info(std::to_string(rewind_injections) + " rewind injections");
    return rewind_injections;
}

std::vector<Packet> Emulation::send_pkt(const Packet &pkt, bool rewinding) {
    std::unique_lock<std::mutex> lck(mtx);
    recv_pkts.clear();
    drop_ts = 0;
    DropMon::get().start_listening_for(pkt);

    Stats::set_pkt_lat_t1();
    env->inject_packet(pkt);

    if (dropmon) { // use drop monitor
        // cv.wait(lck);
        std::chrono::microseconds timeout(5000);
        std::cv_status status = cv.wait_for(lck, timeout);
        Stats::set_pkt_latency(timeout, drop_ts);

        if (status == std::cv_status::timeout && recv_pkts.empty() &&
            drop_ts == 0) {
            Logger::error("Drop monitor timed out!");
        }
    } else if (rewinding) { // use timeout (rewind)
        std::chrono::microseconds timeout(5000);
        std::cv_status status = cv.wait_for(lck, timeout);
        Stats::set_pkt_latency(timeout);

        if (status == std::cv_status::timeout && recv_pkts.empty()) {
            Logger::error("Rewind timed out!");
        }
    } else { // use timeout (new injection)
        std::cv_status status = cv.wait_for(lck, emulated_mb->get_timeout());
        Stats::set_pkt_latency(emulated_mb->get_timeout());

        // logging
        if (status == std::cv_status::timeout && recv_pkts.empty()) {
            // It is possible that the condition variable's timeout occurs after
            // the listening thread has acquired the lock but before it calls
            // the notification function, in which case, the attempt of
            // wait_for's acquiring the lock will block until the listening
            // thread releases it.
            Logger::info("Timed out!");
        }
    }

    DropMon::get().stop_listening();

    /*
     * NOTE:
     * We don't process the read packets in the critical section (i.e., here).
     * Instead, we process the read packets in ForwardingProcess, which is also
     * because the knowledge of the current connection state is required to
     * process it correctly, as mentioned in lib/net.cpp.
     */

    // return the received packets
    return recv_pkts;
}
