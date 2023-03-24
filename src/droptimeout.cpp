#include "droptimeout.hpp"

#include <cmath>
#include <thread>

#include "stats.hpp"

using namespace std;

DropTimeout::DropTimeout()
    : _has_initial_estimate(false), _ntasks(0), _mdev_scalar(0) {}

DropTimeout &DropTimeout::get() {
    static DropTimeout instance;
    return instance;
}

/**
 * It calculates the average and mean deviation of the packet latencies, and
 * stores them in the `_lat_avg` and `_lat_mdev` fields
 */
void DropTimeout::set_initial_latency_estimate() {
    const auto &latencies = Stats::get_pkt_latencies();

    // Calculate the latency average
    uint64_t avg = 0;
    for (const auto &[t1, lat] : latencies) {
        avg += lat;
    }
    avg /= latencies.size();
    this->_lat_avg = chrono::microseconds(avg / 1000); // nsec -> usec

    // Calculate the latency mean deviation
    uint64_t mdev = 0;
    for (const auto &[t1, lat] : latencies) {
        mdev += abs(int64_t(lat) - int64_t(avg));
    }
    mdev /= latencies.size();
    this->_lat_mdev = chrono::microseconds(mdev / 1000); // nsec -> usec

    this->_has_initial_estimate = true;
    Stats::clear_latencies();
}

void DropTimeout::adjust_latency_estimate_by_ntasks(int ntasks) {
    _ntasks = ntasks;
    static const int total_cores = thread::hardware_concurrency();
    double load = double(_ntasks) / total_cores;
    _mdev_scalar = max(4.0, ceil(sqrt(_ntasks) * 2 * load));
    // _lat_avg *= _ntasks;
    _timeout = _lat_avg * _ntasks + _lat_mdev * _mdev_scalar;
}

void DropTimeout::update_timeout(size_t num_recv_pkts) {
    if (num_recv_pkts == 0) {
        return;
    }

    // TODO: Something seems wrong here
    long long err =
        Stats::get_pkt_latencies().back().second / 1000 + 1 - _lat_avg.count();
    _lat_avg += chrono::microseconds(err >> 2);
    _lat_mdev += chrono::microseconds((abs(err) - _lat_mdev.count()) >> 2);
    _timeout = _lat_avg + _lat_mdev * _mdev_scalar;
}
