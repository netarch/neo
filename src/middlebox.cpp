#include "middlebox.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <unordered_map>

#include "emulationmgr.hpp"
#include "payload.hpp"
#include "stats.hpp"

using namespace std;

Middlebox::Middlebox() : _mdev_scalar(0), _emulation(nullptr) {}

void Middlebox::update_timeout() {
    _timeout = _lat_avg + _lat_mdev * _mdev_scalar;
}

void Middlebox::adjust_latency_estimate_by_ntasks(int ntasks) {
    static const int total_cores = thread::hardware_concurrency();
    double load = double(ntasks) / total_cores;
    _mdev_scalar = max(4.0, ceil(sqrt(ntasks) * 2 * load));
    _lat_avg *= ntasks;
    this->update_timeout();
}

int Middlebox::rewind(NodePacketHistory *nph) {
    _emulation = EmulationMgr::get().get_emulation(this, nph);
    return _emulation->rewind(nph);
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph) {
    assert(_emulation->get_mb() == this);
    EmulationMgr::get().update_node_pkt_hist(_emulation, nph);
}

std::list<Packet> Middlebox::send_pkt(const Packet &pkt) {
    assert(_emulation->get_mb() == this);
    std::list<Packet> recv_pkts = _emulation->send_pkt(pkt);

    // if (!recv_pkts.empty() && !dropmon) {
    if (!recv_pkts.empty()) {
        long long err = Stats::get_pkt_latencies().back().second / 1000 + 1 -
                        _lat_avg.count();
        _lat_avg += chrono::microseconds(err >> 2);
        _lat_mdev += chrono::microseconds((abs(err) - _lat_mdev.count()) >> 2);
        this->update_timeout();
    }

    return recv_pkts;
}

std::set<FIB_IPNH>
Middlebox::get_ipnhs(const IPv4Address &dst __attribute__((unused)),
                     const RoutingTable *rib __attribute__((unused)),
                     std::unordered_set<IPv4Address> *looked_up_ips
                     __attribute__((unused))) {
    return std::set<FIB_IPNH>();
}
