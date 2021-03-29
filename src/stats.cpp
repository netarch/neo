#include "stats.hpp"

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fstream>

#include "lib/logger.hpp"
#include "policy/policy.hpp"

using namespace std::chrono;

/********************* private definitions *********************/

bool Stats::record_latencies = false;    // no latency recording by defualt
std::chrono::high_resolution_clock::time_point Stats::total_t1;
std::chrono::microseconds Stats::total_time;
long Stats::total_maxrss;
std::chrono::high_resolution_clock::time_point Stats::policy_t1;
std::chrono::microseconds Stats::policy_time;
long Stats::policy_maxrss;
std::chrono::high_resolution_clock::time_point Stats::ec_t1;
std::chrono::microseconds Stats::ec_time;
long Stats::ec_maxrss;
std::chrono::high_resolution_clock::time_point Stats::overall_lat_t1;
std::chrono::high_resolution_clock::time_point Stats::rewind_lat_t1;
std::chrono::high_resolution_clock::time_point Stats::pkt_lat_t1;
std::vector<std::chrono::nanoseconds>   Stats::overall_latencies;
std::vector<std::chrono::nanoseconds>   Stats::rewind_latencies;
std::vector<int>                        Stats::rewind_injection_count;
std::vector<std::chrono::nanoseconds>   Stats::pkt_latencies;

high_resolution_clock::duration
Stats::get_duration(const high_resolution_clock::time_point& t1)
{
    if (t1.time_since_epoch().count() == 0) {
        Logger::error("t1 not set");
    }

    return high_resolution_clock::now() - t1;
}

/********************* control functions *********************/

void Stats::enable_latency_recording()
{
    record_latencies = true;
}

void Stats::output_main_stats()
{
    Logger::info("====================");
    Logger::info("Time: " + std::to_string(total_time.count()) + " microseconds");
    Logger::info("Memory: " + std::to_string(total_maxrss) + " kilobytes");
}

void Stats::output_policy_stats(int nodes, int links, Policy *policy)
{
    const std::string filename = "policy.stats.csv";
    std::ofstream outfile(filename, std::ios_base::app);
    if (outfile.fail()) {
        Logger::error("Failed to open " + filename);
    }

    outfile << "# of nodes, # of links, Policy, # of connection ECs, "
            "Time (microseconds), Memory (kilobytes)" << std::endl
            << nodes << ", "
            << links << ", "
            << policy->get_id() << ", "
            << policy->num_conn_ecs() << ", "
            << policy_time.count() << ", "
            << policy_maxrss << std::endl;
}

void Stats::output_ec_stats()
{
    const std::string filename = std::to_string(getpid()) + ".stats.csv";
    std::ofstream outfile(filename, std::ios_base::app);
    if (outfile.fail()) {
        Logger::error("Failed to open " + filename);
    }

    outfile << "Time (microseconds), Memory (kilobytes)" << std::endl
            << ec_time.count() << ", " << ec_maxrss << std::endl;

    if (record_latencies) {
        outfile << "Overall latency (nanoseconds), "
                "Rewind latency (nanoseconds), "
                "Rewind injection count, "
                "Packet latency (nanoseconds)" << std::endl;
        for (size_t i = 0; i < overall_latencies.size(); ++i) {
            outfile << overall_latencies[i].count() << ", "
                    << rewind_latencies[i].count() << ", "
                    << rewind_injection_count[i] << ", "
                    << pkt_latencies[i].count() << std::endl;
        }
    }
}

/********************* main process measurements *********************/

void Stats::set_total_t1()
{
    total_t1 = high_resolution_clock::now();
}

void Stats::set_total_time()
{
    total_time = duration_cast<microseconds>(get_duration(total_t1));
}

void Stats::set_total_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_CHILDREN, &ru) < 0) {
        Logger::error("getrusage()", errno);
    }
    total_maxrss = ru.ru_maxrss;
}

/******************** policy process measurements ********************/

void Stats::set_policy_t1()
{
    policy_t1 = high_resolution_clock::now();
}

void Stats::set_policy_time()
{
    policy_time = duration_cast<microseconds>(get_duration(policy_t1));
}

void Stats::set_policy_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_CHILDREN, &ru) < 0) {
        Logger::error("getrusage()", errno);
    }
    policy_maxrss = ru.ru_maxrss;
}

/********************** EC process measurements **********************/

void Stats::set_ec_t1()
{
    ec_t1 = high_resolution_clock::now();
}

void Stats::set_ec_time()
{
    ec_time = duration_cast<microseconds>(get_duration(ec_t1));
}

void Stats::set_ec_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) < 0) {
        Logger::error("getrusage()", errno);
    }
    ec_maxrss = ru.ru_maxrss;
}

/********************** latency measurements **********************/

void Stats::set_overall_lat_t1()
{
    if (record_latencies) {
        overall_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_overall_latency()
{
    if (record_latencies) {
        auto overall_latency = duration_cast<nanoseconds>(get_duration(overall_lat_t1));
        overall_latencies.push_back(overall_latency);
    }
}

void Stats::set_rewind_lat_t1()
{
    if (record_latencies) {
        rewind_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_rewind_latency()
{
    if (record_latencies) {
        auto rewind_latency = duration_cast<nanoseconds>(get_duration(rewind_lat_t1));
        rewind_latencies.push_back(rewind_latency);
    }
}

void Stats::set_rewind_injection_count(int count)
{
    if (record_latencies) {
        rewind_injection_count.push_back(count);
    }
}

void Stats::set_pkt_lat_t1()
{
    if (record_latencies) {
        pkt_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_pkt_latency()
{
    if (record_latencies) {
        auto pkt_latency = duration_cast<nanoseconds>(get_duration(pkt_lat_t1));
        pkt_latencies.push_back(pkt_latency);
    }
}
