#include "middlebox.hpp"

#include <cassert>
#include <cstdlib>

#include "stats.hpp"
#include "emulationmgr.hpp"

Middlebox::Middlebox()
    : emulation(nullptr), app(nullptr)
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

void Middlebox::increase_latency_estimate_by_DOP(size_t DOP)
{
    latency_avg *= DOP;
    latency_mdev *= DOP;
    timeout = latency_avg + latency_mdev * 4;
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
    if (!recv_pkts.empty()) {
        long long err = Stats::get_pkt_latencies().back().count() / 1000 - latency_avg.count();
        latency_avg += std::chrono::microseconds(err >> 2);
        latency_mdev += std::chrono::microseconds((std::abs(err) - latency_mdev.count()) >> 2);
        timeout = latency_avg + latency_mdev * 4;
        Logger::debug("Drop timeout: " + std::to_string(timeout.count()));
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
