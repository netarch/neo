#pragma once

#include <chrono>

/**
 * @brief Packet injection latency and drop timeout estimate
 */
class DropTimeout {
private:
    std::chrono::microseconds _lat_avg, _lat_mdev, _timeout;
    bool _has_initial_estimate;
    int _ntasks, _mdev_scalar;

private:
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
    bool has_initial_estimate() const { return _has_initial_estimate; }
    decltype(_ntasks) ntasks() const { return _ntasks; }
    decltype(_mdev_scalar) mdev_scalar() const { return _mdev_scalar; }

    void set_initial_latency_estimate();
    void adjust_latency_estimate_by_ntasks(int ntasks);
    void update_timeout(size_t num_recv_pkts);
};
