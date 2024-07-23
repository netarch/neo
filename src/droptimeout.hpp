#pragma once

#include <chrono>

/**
 * @brief Packet injection latency and drop timeout estimate
 */
class DropTimeout {
private:
    std::chrono::microseconds _lat_avg, _lat_mdev, _timeout;
    bool _has_initial_estimate;
    int _nprocs, _mdev_scalar;

private:
    friend class ConfigParser;
    DropTimeout();

public:
    // Disable the copy/move constructors and the assignment operators
    DropTimeout(const DropTimeout &) = delete;
    DropTimeout(DropTimeout &&) = delete;
    DropTimeout &operator=(const DropTimeout &) = delete;
    DropTimeout &operator=(DropTimeout &&) = delete;

    static DropTimeout &get();

    decltype(_lat_avg) lat_avg() const { return _lat_avg; }
    decltype(_lat_mdev) lat_mdev() const { return _lat_mdev; }
    decltype(_timeout) timeout() const { return _timeout; }
    decltype(_nprocs) nprocs() const { return _nprocs; }
    decltype(_mdev_scalar) mdev_scalar() const { return _mdev_scalar; }

    void init();
    void reset();
    void adjust_latency_estimate_by_nprocs(int nprocs);
    void update_timeout();
};
