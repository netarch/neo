#pragma once

#include <vector>
#include <chrono>
#include <string>

#include "policy/policy.hpp"

class Stats
{
private:
    Stats() = default;
    std::chrono::high_resolution_clock::duration
    get_time(const std::chrono::high_resolution_clock::time_point&);

    bool latencies;   // true if latencies should be recorded

    /*
     * main process measurements
     */
    std::chrono::high_resolution_clock::time_point total_t1;
    std::chrono::microseconds   total_time;
    long                        total_maxrss;   // kilobytes

    /*
     * policy process measurements
     */
    std::chrono::high_resolution_clock::time_point policy_t1;
    std::chrono::microseconds   policy_time;
    long                        policy_maxrss;  // kilobytes

    /*
     * EC process measurements
     */
    std::chrono::high_resolution_clock::time_point ec_t1;
    std::chrono::microseconds   ec_time;
    long                        ec_maxrss;  // kilobytes

    std::chrono::high_resolution_clock::time_point overall_lat_t1;
    std::chrono::high_resolution_clock::time_point rewind_lat_t1;
    std::chrono::high_resolution_clock::time_point pkt_lat_t1;

    // time for ForwardingProcess::inject_packet()
    std::vector<std::chrono::nanoseconds>   overall_latencies;
    // time for rewinding the middlebox state
    std::vector<std::chrono::nanoseconds>   rewind_latencies;
    // number of packet injections when rewinding (-1 means no rewind occured)
    std::vector<int>                        rewind_injection_count;
    // time for injecting the actual target packet
    std::vector<std::chrono::nanoseconds>   pkt_latencies;

public:
    // Disable the copy constructor and the copy assignment operator
    Stats(const Stats&) = delete;
    Stats& operator=(const Stats&) = delete;

    static Stats& get();

    void record_latencies(bool);
    void print_ec_stats();
    void print_policy_stats(int nodes, int links, Policy *);
    void print_main_stats();

    /*
     * main process measurements
     */
    void set_total_t1();
    void set_total_time();
    void set_total_maxrss();

    /*
     * policy process measurements
     */
    void set_policy_t1();
    void set_policy_time();
    void set_policy_maxrss();

    /*
     * EC process measurements
     */
    void set_ec_t1();
    void set_ec_time();
    void set_ec_maxrss();

    void set_overall_lat_t1();
    void set_overall_latency();
    void set_rewind_lat_t1();
    void set_rewind_latency();
    void set_rewind_injection_count(int);
    void set_pkt_lat_t1();
    void set_pkt_latency();
};
