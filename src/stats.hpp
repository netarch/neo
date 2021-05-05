#pragma once

#include <vector>
#include <chrono>
#include <map>

class Policy;

class Stats
{
private:
    /*
     * main process measurements
     */
    static std::chrono::high_resolution_clock::time_point total_t1;
    static std::chrono::microseconds   total_time;
    static long                        total_maxrss;   // kilobytes

    /*
     * policy process measurements
     */
    static std::chrono::high_resolution_clock::time_point policy_t1;
    static std::chrono::microseconds   policy_time;
    static long                        policy_maxrss;  // kilobytes

    /*
     * EC process measurements
     */
    static std::chrono::high_resolution_clock::time_point ec_t1;
    static std::chrono::microseconds   ec_time;
    static long                        ec_maxrss;  // kilobytes

    /*
     * latency measurements (nanoseconds)
     */
    static uint64_t overall_lat_t1;
    static uint64_t rewind_lat_t1;
    static uint64_t pkt_lat_t1;
    // time for ForwardingProcess::inject_packet()
    static std::vector<std::pair<uint64_t, uint64_t>>   overall_latencies;
    // time for rewinding the middlebox state
    static std::vector<std::pair<uint64_t, uint64_t>>   rewind_latencies;
    // number of packet injections when rewinding (-1 means no rewind occured)
    static std::vector<int>                             rewind_injection_count;
    // time between injecting the actual target packet and getting the result
    static std::vector<std::pair<uint64_t, uint64_t>>   pkt_latencies;
    // actual timeout values used
    static std::vector<uint64_t>                        timeouts;
    // time between injecting the packet and getting dropped in kernel
    static std::map<uint64_t, uint64_t>                 kernel_drop_latencies;

    /*
     * helper functions
     */
    static std::chrono::high_resolution_clock::duration
    get_duration(const std::chrono::high_resolution_clock::time_point&);
    static uint64_t get_duration(uint64_t);

public:
    /*
     * control functions
     */
    static void output_main_stats();
    static void output_policy_stats(int nodes, int links, Policy *);
    static void output_ec_stats();
    static void clear_latencies();

    /*
     * main process measurements
     */
    static void set_total_t1();
    static void set_total_time();
    static void set_total_maxrss();

    /*
     * policy process measurements
     */
    static void set_policy_t1();
    static void set_policy_time();
    static void set_policy_maxrss();

    /*
     * EC process measurements
     */
    static void set_ec_t1();
    static void set_ec_time();
    static void set_ec_maxrss();

    /*
     * latency measurements
     */
    static void set_overall_lat_t1();
    static void set_overall_latency();
    static void set_rewind_lat_t1();
    static void set_rewind_latency();
    static void set_rewind_injection_count(int);
    static void set_pkt_lat_t1();
    static void set_pkt_latency(
        const std::chrono::high_resolution_clock::duration& timeout,
        uint64_t drop_ts = 0);

    /*
     * getter functions
     */
    static const std::vector<std::pair<uint64_t, uint64_t>>& get_pkt_latencies();
};
