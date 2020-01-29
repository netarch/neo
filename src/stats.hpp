#pragma once

#include <vector>
#include <chrono>

#include "policy/policy.hpp"

class Stats
{
private:
    Stats() = default;
    std::chrono::high_resolution_clock::duration
    get_time(const std::chrono::high_resolution_clock::time_point&);

    /*
     * main process measurements
     */
    std::chrono::high_resolution_clock::time_point policy_t1;
    std::chrono::high_resolution_clock::time_point total_t1;
    std::vector<std::chrono::microseconds> policy_times;
    std::chrono::microseconds   total_time;
    long                        total_maxrss;   // kilobytes

    /*
     * per process (per EC) measurements
     */
    std::chrono::high_resolution_clock::time_point verify_t1;
    std::chrono::microseconds   verify_time;
    long                        verify_maxrss;  // kilobytes

    std::chrono::high_resolution_clock::time_point overall_lat_t1;
    std::chrono::high_resolution_clock::time_point rewind_lat_t1;
    std::chrono::high_resolution_clock::time_point pkt_lat_t1;

    // time for ForwardingProcess::inject_packet()
    std::vector<std::chrono::nanoseconds>   overall_latencies;
    // time for rewinding the middlebox state
    std::vector<std::chrono::nanoseconds>   rewind_latencies;
    // number of packet injections when rewinding
    std::vector<int>                        rewind_injection_count;
    // time for injecting the actual target packet
    std::vector<std::chrono::nanoseconds>   pkt_latencies;

public:
    // Disable the copy constructor and the copy assignment operator
    Stats(const Stats&) = delete;
    Stats& operator=(const Stats&) = delete;

    static Stats& get();

    void print_per_process_stats();
    void print_main_stats(int nodes, int links, const Policies&);

    /*
     * main process measurements
     */
    void set_policy_t1();
    void set_policy_time();
    void set_total_t1();
    void set_total_time();
    void set_total_maxrss();

    /*
     * per process (per EC) measurements
     */
    void set_verify_t1();
    void set_verify_time();
    void set_verify_maxrss();

    void set_overall_lat_t1();
    void set_overall_latency();
    void set_rewind_lat_t1();
    void set_rewind_latency();
    void set_rewind_injection_count(int);
    void set_pkt_lat_t1();
    void set_pkt_latency();
};
