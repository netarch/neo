#include "droptimeout.hpp"

#include <cmath>
#include <thread>

#include "configparser.hpp"
#include "stats.hpp"

using namespace std;

DropTimeout::DropTimeout() :
    _has_initial_estimate(false),
    _nprocs(0),
    _mdev_scalar(0) {}

DropTimeout &DropTimeout::get() {
    static DropTimeout instance;
    return instance;
}

/**
 * It calculates the average and mean deviation of the packet latencies, and
 * stores them in the `_lat_avg` and `_lat_mdev` fields
 */
void DropTimeout::init() {
    if (_has_initial_estimate) {
        return;
    }

    _STATS_RESET();
    ConfigParser().estimate_pkt_lat(20);
    const auto &latencies = Stats::get().get_pkt_latencies();

    // Calculate the latency average
    chrono::microseconds avg{0};
    for (const auto &lat : latencies) {
        avg += lat;
    }
    avg /= latencies.size();
    this->_lat_avg = avg;

    // Calculate the latency mean deviation
    chrono::microseconds mdev{0};
    for (const auto &lat : latencies) {
        mdev += chrono::abs(lat - avg);
    }
    mdev /= latencies.size();
    this->_lat_mdev = mdev;

    this->_has_initial_estimate = true;
    _STATS_RESET();
}

void DropTimeout::reset() {
    _lat_avg              = chrono::microseconds();
    _lat_mdev             = chrono::microseconds();
    _timeout              = chrono::microseconds();
    _has_initial_estimate = false;
    _nprocs               = 0;
    _mdev_scalar          = 0;
}

/**
 * The function adjusts the timeout value based on the number of parallel
 * processes.
 *
 * @param nprocs the number of parallel processes for the current invariant
 */
void DropTimeout::adjust_latency_estimate_by_nprocs(int nprocs) {
    _nprocs                      = nprocs;
    static const int total_cores = thread::hardware_concurrency();
    double load                  = double(_nprocs) / total_cores;
    _mdev_scalar                 = max(5.0, ceil(sqrt(_nprocs) * 2 * load));
    _timeout                     = _lat_avg + _lat_mdev * _mdev_scalar;
}

/**
 * @brief Update the average and mean deviation of latencies based on the latest
 * packet injection, and then update the timeout accordingly.
 *
 * We estimate the latency average and mean deviation (as a measure for
 * variance) similar to the TCP retransmit timeout estimation.
 * avg := avg + \alpha * err.
 * mdev := mdev + \beta * (|err| - mdev).
 * timeout := avg + C * mdev.
 * where \alpha and \beta are the "gain", denoting the weight of the new
 * latency. A larger gain makes the estimate react faster to latency changes but
 * also makes it more sensitive to noise. C is the mdev scalar, which the TCP
 * paper set to 1/gain, but here we make it to be proportional to the load and
 * the number of parallel processes.
 *
 * Configuration:
 * \alpha: 1/5
 * \beta: 1/5
 *
 * References:
 * Congestion avoidance and control (https://dl.acm.org/doi/10.1145/52324.52356)
 * RFC 793 (https://www.rfc-editor.org/rfc/rfc793)
 * RFC 1122 (https://datatracker.ietf.org/doc/html/rfc1122)
 * RFC 6298 (https://datatracker.ietf.org/doc/html/rfc6298)
 * RFC 9293 (https://datatracker.ietf.org/doc/html/rfc9293)
 */
void DropTimeout::update_timeout() {
    auto err = Stats::get().get_pkt_latencies().back() - _lat_avg;
    _lat_avg += err / 5;
    _lat_mdev += (chrono::abs(err) - _lat_mdev) / 5;
    _timeout = _lat_avg + _lat_mdev * _mdev_scalar;
}
