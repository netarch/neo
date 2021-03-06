#include "middlebox.hpp"

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <algorithm>

#include "stats.hpp"
#include "emulationmgr.hpp"

Middlebox::Middlebox()
    : emulation(nullptr), app(nullptr), dropmon(false), dev_scalar(0)
{
}

Middlebox::~Middlebox()
{
    delete app;
}

std::string Middlebox::get_env() const
{
    return env;
}

MB_App *Middlebox::get_app() const
{
    return app;
}

std::chrono::microseconds Middlebox::get_timeout() const
{
    return timeout;
}

bool Middlebox::dropmon_enabled() const
{
    return dropmon;
}

void Middlebox::update_timeout()
{
    timeout = latency_avg + latency_mdev * dev_scalar;
}

void Middlebox::increase_latency_estimate_by_DOP(int DOP)
{
    static const int total_cores = std::thread::hardware_concurrency();
    double load = (double)DOP / total_cores;
    dev_scalar = std::max(4.0, ceil(sqrt(DOP) * 2 * load));
    latency_avg *= DOP;
    update_timeout();
}

int Middlebox::rewind(NodePacketHistory *nph)
{
    emulation = EmulationMgr::get().get_emulation(this, nph);
    return emulation->rewind(nph);
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    assert(emulation->get_mb() == this);
    EmulationMgr::get().update_node_pkt_hist(emulation, nph);
}

std::vector<Packet> Middlebox::send_pkt(const Packet& pkt)
{
    assert(emulation->get_mb() == this);
    std::vector<Packet> recv_pkts = emulation->send_pkt(pkt);
    if (!recv_pkts.empty() && !dropmon) {
        long long err = Stats::get_pkt_latencies().back().second / 1000 + 1 - latency_avg.count();
        latency_avg += std::chrono::microseconds(err >> 2);
        latency_mdev += std::chrono::microseconds((std::abs(err) - latency_mdev.count()) >> 2);
        update_timeout();
    }
    return recv_pkts;
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)),
    const RoutingTable *rib __attribute__((unused)),
    std::unordered_set<IPv4Address> *looked_up_ips __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
