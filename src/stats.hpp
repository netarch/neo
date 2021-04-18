#pragma once

#include <vector>
#include <chrono>
#include <string>

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
     * latency measurements
     */
    static std::chrono::high_resolution_clock::time_point overall_lat_t1;
    static std::chrono::high_resolution_clock::time_point rewind_lat_t1;
    static std::chrono::high_resolution_clock::time_point pkt_lat_t1;
    // time for ForwardingProcess::inject_packet()
    static std::vector<std::chrono::nanoseconds>   overall_latencies;
    // time for rewinding the middlebox state
    static std::vector<std::chrono::nanoseconds>   rewind_latencies;
    // number of packet injections when rewinding (-1 means no rewind occured)
    static std::vector<int>                        rewind_injection_count;
    // time between injecting the actual target packet and getting the result
    static std::vector<std::chrono::nanoseconds>   pkt_latencies;

    /*
     * helper functions
     */
    static std::chrono::high_resolution_clock::duration
    get_duration(const std::chrono::high_resolution_clock::time_point&);

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
    static void set_pkt_latency();

    /*
     * getter functions
     */
    static const std::vector<std::chrono::nanoseconds>& get_pkt_latencies();
};
