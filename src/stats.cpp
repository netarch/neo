#include "stats.hpp"

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fstream>

#include "lib/logger.hpp"
#include "policy/policy.hpp"

using namespace std::chrono;

/********************* private definitions *********************/

std::chrono::high_resolution_clock::time_point Stats::total_t1;
std::chrono::microseconds Stats::total_time;
long Stats::total_maxrss;
std::chrono::high_resolution_clock::time_point Stats::policy_t1;
std::chrono::microseconds Stats::policy_time;
long Stats::policy_maxrss;
std::chrono::high_resolution_clock::time_point Stats::ec_t1;
std::chrono::microseconds Stats::ec_time;
long Stats::ec_maxrss;
uint64_t Stats::overall_lat_t1;
uint64_t Stats::rewind_lat_t1;
uint64_t Stats::pkt_lat_t1;
std::vector<std::pair<uint64_t, uint64_t>>  Stats::overall_latencies;
std::vector<std::pair<uint64_t, uint64_t>>  Stats::rewind_latencies;
std::vector<int>                            Stats::rewind_injection_count;
std::vector<std::pair<uint64_t, uint64_t>>  Stats::pkt_latencies;
std::vector<std::pair<uint64_t, uint64_t>>  Stats::kernel_drop_latencies;

high_resolution_clock::duration
Stats::get_duration(const high_resolution_clock::time_point& t1)
{
    if (t1.time_since_epoch().count() == 0) {
        Logger::error("t1 not set");
    }
    return high_resolution_clock::now() - t1;
}

uint64_t Stats::get_duration(uint64_t t1)
{
    if (t1 == 0) {
        Logger::error("t1 not set");
    }
    return duration_cast<nanoseconds>(
               high_resolution_clock::now().time_since_epoch()
           ).count() - t1;
}

/********************* control functions *********************/

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

    outfile << "Overall latency t1 (nsec), Overall latency (nsec), "
            << "Rewind latency t1 (nsec), Rewind latency (nsec), "
            << "Rewind injection count, "
            << "Packet latency t1 (nsec), Packet latency (nsec)"
            << std::endl;
    for (size_t i = 0; i < overall_latencies.size(); ++i) {
        outfile << overall_latencies[i].first << ", "
                << overall_latencies[i].second << ", "
                << rewind_latencies[i].first << ", "
                << rewind_latencies[i].second << ", "
                << rewind_injection_count[i] << ", "
                << pkt_latencies[i].first << ", "
                << pkt_latencies[i].second << std::endl;
    }

    outfile << "Kernel drop latency t1 (nsec), Kernel drop latency (nsec)"
            << std::endl;
    for (size_t i = 0; i < kernel_drop_latencies.size(); ++i) {
        outfile << kernel_drop_latencies[i].first << ", "
                << kernel_drop_latencies[i].second << std::endl;
    }
}

void Stats::clear_latencies()
{
    overall_latencies.clear();
    rewind_latencies.clear();
    rewind_injection_count.clear();
    pkt_latencies.clear();
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
    overall_lat_t1 = duration_cast<nanoseconds>(
                         high_resolution_clock::now().time_since_epoch()
                     ).count();
}

void Stats::set_overall_latency()
{
    uint64_t overall_latency = get_duration(overall_lat_t1);
    overall_latencies.emplace_back(overall_lat_t1, overall_latency);
}

void Stats::set_rewind_lat_t1()
{
    rewind_lat_t1 = duration_cast<nanoseconds>(
                        high_resolution_clock::now().time_since_epoch()
                    ).count();
}

void Stats::set_rewind_latency()
{
    uint64_t rewind_latency = get_duration(rewind_lat_t1);
    rewind_latencies.emplace_back(rewind_lat_t1, rewind_latency);
}

void Stats::set_rewind_injection_count(int count)
{
    rewind_injection_count.push_back(count);
}

void Stats::set_pkt_lat_t1()
{
    pkt_lat_t1 = duration_cast<nanoseconds>(
                     high_resolution_clock::now().time_since_epoch()
                 ).count();
}

void Stats::set_pkt_latency(uint64_t drop_ts)
{
    uint64_t pkt_latency = get_duration(pkt_lat_t1);
    pkt_latencies.emplace_back(pkt_lat_t1, pkt_latency);
    if (drop_ts) {
        Logger::debug("drop_ts:    " + std::to_string(drop_ts));
        Logger::debug("pkt_lat_t1: " + std::to_string(pkt_lat_t1));
        kernel_drop_latencies.emplace_back(pkt_lat_t1, drop_ts - pkt_lat_t1);
    }
}

/************************* getter functions *************************/

const std::vector<std::pair<uint64_t, uint64_t>>& Stats::get_pkt_latencies()
{
    return Stats::pkt_latencies;
}
