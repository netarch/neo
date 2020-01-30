#include "stats.hpp"

#include <sys/time.h>
#include <sys/resource.h>

#include "lib/logger.hpp"

using namespace std::chrono;

Stats& Stats::get()
{
    static Stats instance;
    return instance;
}

void Stats::record_latencies(bool l)
{
    latencies = l;
}

void Stats::print_ec_stats()
{
    Logger::get().print("Time (microseconds), Memory (kilobytes)");
    Logger::get().print(std::to_string(ec_time.count()) + ", " +
                        std::to_string(ec_maxrss));
    if (latencies) {
        Logger::get().print("Overall latency (nanoseconds), "
                            "Rewind latency (nanoseconds), "
                            "Rewind injection count, "
                            "Packet latency (nanoseconds)");
        for (size_t i = 0; i < overall_latencies.size(); ++i) {
            Logger::get().print(
                std::to_string(overall_latencies[i].count()) + ", " +
                std::to_string(rewind_latencies[i].count()) + ", " +
                std::to_string(rewind_injection_count[i]) + ", " +
                std::to_string(pkt_latencies[i].count()));
        }
    }
}

void Stats::print_policy_stats(int nodes, int links, Policy *policy)
{
    Logger::get().print("# of nodes, # of links, Policy, # of ECs, "
                        "Time (microseconds), Memory (kilobytes)");
    Logger::get().print(
        std::to_string(nodes) + ", " +
        std::to_string(links) + ", " +
        std::to_string(policy->get_id()) + ", " +
        std::to_string(policy->num_ecs()) + ", " +
        std::to_string(policy_time.count()) + ", " +
        std::to_string(policy_maxrss));
}

void Stats::print_main_stats()
{
    Logger::get().info("====================");
    Logger::get().info("Time: " + std::to_string(total_time.count()) +
                       " microseconds");
    Logger::get().info("Memory: " + std::to_string(total_maxrss) +
                       " kilobytes");
}

high_resolution_clock::duration
Stats::get_time(const high_resolution_clock::time_point& t1)
{
    if (t1.time_since_epoch().count() == 0) {
        Logger::get().err("t1 not set");
    }

    return high_resolution_clock::now() - t1;
}

/********************* main process measurements *********************/

void Stats::set_total_t1()
{
    total_t1 = high_resolution_clock::now();
}

void Stats::set_total_time()
{
    total_time = duration_cast<microseconds>(get_time(total_t1));
}

void Stats::set_total_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_CHILDREN, &ru) < 0) {
        Logger::get().err("getrusage()", errno);
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
    policy_time = duration_cast<microseconds>(get_time(policy_t1));
}

void Stats::set_policy_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_CHILDREN, &ru) < 0) {
        Logger::get().err("getrusage()", errno);
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
    ec_time = duration_cast<microseconds>(get_time(ec_t1));
}

void Stats::set_ec_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) < 0) {
        Logger::get().err("getrusage()", errno);
    }
    ec_maxrss = ru.ru_maxrss;
}

void Stats::set_overall_lat_t1()
{
    if (latencies) {
        overall_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_overall_latency()
{
    if (latencies) {
        auto overall_latency = duration_cast<nanoseconds>(get_time(overall_lat_t1));
        overall_latencies.push_back(overall_latency);
    }
}

void Stats::set_rewind_lat_t1()
{
    if (latencies) {
        rewind_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_rewind_latency()
{
    if (latencies) {
        auto rewind_latency = duration_cast<nanoseconds>(get_time(rewind_lat_t1));
        rewind_latencies.push_back(rewind_latency);
    }
}

void Stats::set_rewind_injection_count(int count)
{
    if (latencies) {
        rewind_injection_count.push_back(count);
    }
}

void Stats::set_pkt_lat_t1()
{
    if (latencies) {
        pkt_lat_t1 = high_resolution_clock::now();
    }
}

void Stats::set_pkt_latency()
{
    if (latencies) {
        auto pkt_latency = duration_cast<nanoseconds>(get_time(pkt_lat_t1));
        pkt_latencies.push_back(pkt_latency);
    }
}
