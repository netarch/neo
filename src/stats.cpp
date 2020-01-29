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

void Stats::print_per_process_stats()
{
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
    Logger::get().print("Time (microseconds), Peak Memory (kilobytes)");
    Logger::get().print(std::to_string(verify_time.count()) + ", " +
                        std::to_string(verify_maxrss));
}

void Stats::print_main_stats(int nodes, int links, const Policies& policies)
{
    Logger::get().print("# of nodes, # of links, Policy, # of ECs, "
                        "Time (microseconds), Peak Memory (kilobytes)");
    for (Policy * const& policy : policies) {
        Logger::get().print(
            std::to_string(nodes) + ", " +
            std::to_string(links) + ", " +
            std::to_string(policy->get_id()) + ", " +
            std::to_string(policy->num_ecs()) + ", " +
            std::to_string(policy_times[policy->get_id() - 1].count()) +
            ", N/A");
    }
    Logger::get().print(
        std::to_string(nodes) + ", " +
        std::to_string(links) + ", "
        "all, N/A, " +
        std::to_string(total_time.count()) + ", " +
        std::to_string(total_maxrss));
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

void Stats::set_policy_t1()
{
    policy_t1 = high_resolution_clock::now();
}

void Stats::set_policy_time()
{
    auto policy_time = duration_cast<microseconds>(get_time(policy_t1));
    policy_times.push_back(policy_time);
}

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

/***************** per process (per EC) measurements *****************/

void Stats::set_verify_t1()
{
    verify_t1 = high_resolution_clock::now();
}

void Stats::set_verify_time()
{
    verify_time = duration_cast<microseconds>(get_time(verify_t1));
}

void Stats::set_verify_maxrss()
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) < 0) {
        Logger::get().err("getrusage()", errno);
    }
    verify_maxrss = ru.ru_maxrss;
}

void Stats::set_overall_lat_t1()
{
    overall_lat_t1 = high_resolution_clock::now();
}

void Stats::set_overall_latency()
{
    auto overall_latency = duration_cast<nanoseconds>(get_time(overall_lat_t1));
    overall_latencies.push_back(overall_latency);
}

void Stats::set_rewind_lat_t1()
{
    rewind_lat_t1 = high_resolution_clock::now();
}

void Stats::set_rewind_latency()
{
    auto rewind_latency = duration_cast<nanoseconds>(get_time(rewind_lat_t1));
    rewind_latencies.push_back(rewind_latency);
}

void Stats::set_rewind_injection_count(int count)
{
    rewind_injection_count.push_back(count);
}

void Stats::set_pkt_lat_t1()
{
    pkt_lat_t1 = high_resolution_clock::now();
}

void Stats::set_pkt_latency()
{
    auto pkt_latency = duration_cast<nanoseconds>(get_time(pkt_lat_t1));
    pkt_latencies.push_back(pkt_latency);
}
