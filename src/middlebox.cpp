#include "middlebox.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <unordered_map>

#include "emulationmgr.hpp"
#include "lib/net.hpp"
#include "payload.hpp"
#include "stats.hpp"

Middlebox::Middlebox()
    : emulation(nullptr), app(nullptr), dropmon(false), dev_scalar(0) {}

Middlebox::~Middlebox() {
    delete app;
}

Emulation *Middlebox::get_emulation() const {
    assert(this->emulation->get_mb() == this);
    return this->emulation;
}

std::string Middlebox::get_env() const {
    return env;
}

MB_App *Middlebox::get_app() const {
    return app;
}

std::chrono::microseconds Middlebox::get_timeout() const {
    return timeout;
}

bool Middlebox::dropmon_enabled() const {
    return dropmon;
}

void Middlebox::update_timeout() {
    timeout = latency_avg + latency_mdev * dev_scalar;
}

void Middlebox::increase_latency_estimate_by_DOP(int DOP) {
    static const int total_cores = std::thread::hardware_concurrency();
    double load = (double)DOP / total_cores;
    dev_scalar = std::max(4.0, ceil(sqrt(DOP) * 2 * load));
    latency_avg *= DOP;
    update_timeout();
}

int Middlebox::rewind(State *state, NodePacketHistory *nph) {
    emulation = EmulationMgr::get().get_emulation(this, nph);
    return emulation->rewind(state, nph);
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph) {
    assert(emulation->get_mb() == this);
    EmulationMgr::get().update_node_pkt_hist(emulation, nph);
}

std::list<Packet> Middlebox::send_pkt(const Packet &pkt) {
    assert(emulation->get_mb() == this);
    std::list<Packet> recv_pkts = emulation->send_pkt(pkt);

    if (!recv_pkts.empty() && !dropmon) {
        long long err = Stats::get_pkt_latencies().back().second / 1000 + 1 -
                        latency_avg.count();
        latency_avg += std::chrono::microseconds(err >> 2);
        latency_mdev += std::chrono::microseconds(
            (std::abs(err) - latency_mdev.count()) >> 2);
        this->update_timeout();
    }

    this->reassemble_segments(recv_pkts);
    return recv_pkts;
}

void Middlebox::reassemble_segments(std::list<Packet> &pkts) {
    std::unordered_map<Interface *, std::list<Packet>> intf_pkts_map;
    auto p = pkts.begin();

    while (p != pkts.end()) {
        auto &intf_pkts = intf_pkts_map[p->get_intf()];

        if (!intf_pkts.empty()) {
            auto &last_pkt = intf_pkts.back();

            if (last_pkt.get_src_ip() == p->get_src_ip() &&
                last_pkt.get_dst_ip() == p->get_dst_ip() &&
                last_pkt.get_src_port() == p->get_src_port() &&
                last_pkt.get_dst_port() == p->get_dst_port() &&
                Net::get().is_tcp_ack_or_psh_ack(last_pkt) &&
                Net::get().is_tcp_ack_or_psh_ack(*p) &&
                last_pkt.get_payload()->get_size() > 0 &&
                last_pkt.get_seq() + last_pkt.get_payload()->get_size() ==
                    p->get_seq() &&
                last_pkt.get_ack() == p->get_ack()) {

                last_pkt.set_payload(
                    PayloadMgr::get().get_payload(last_pkt, *p));
                pkts.erase(p++);
                continue;
            }
        }

        intf_pkts.emplace_back(std::move(*p));
        pkts.erase(p++);
    }

    assert(pkts.empty());

    for (auto &[intf, intf_pkts] : intf_pkts_map) {
        pkts.splice(pkts.end(), intf_pkts);
    }
}

std::set<FIB_IPNH>
Middlebox::get_ipnhs(const IPv4Address &dst __attribute__((unused)),
                     const RoutingTable *rib __attribute__((unused)),
                     std::unordered_set<IPv4Address> *looked_up_ips
                     __attribute__((unused))) {
    return std::set<FIB_IPNH>();
}
