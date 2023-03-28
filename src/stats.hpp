#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define _STATS_START(op) Stats::get().start(op)
#define _STATS_STOP(op) Stats::get().stop(op)
#define _STATS_REWIND_INJECTION(n) Stats::get().set_rewind_injection_count(n)
#define _STATS_RESET() Stats::get().reset()
#define _STATS_LOGRESULTS(op) Stats::get().log_results(op)

class Policy;

class Stats {
public:
    using clock = std::chrono::high_resolution_clock;
    enum class Op {
        MAIN_PROC,           // Main process
        CHECK_INVARIANT,     // Checking one invariant
        CHECK_EC,            // Checking one connection EC
        __OP_TYPE_DIVIDER__, // Separating time and packet latency type ops
        FWD_INJECT_PKT,      // ForwardingProcess::inject_packet()
        REWIND,              // Rewinding the middlebox emulation
        PKT_LAT,  // Between sending the packet and getting the response
        DROP_LAT, // Between sending the packet and getting dropped
        TIMEOUT,  // Actual timeout value for packet drops
    };
    class OpHasher {
    public:
        size_t operator()(const Op &op) const {
            return std::hash<int>()(static_cast<int>(op));
        }
    };

private:
    const std::vector<Op> _all_ops = {
        Op::MAIN_PROC,      Op::CHECK_INVARIANT,
        Op::CHECK_EC,       Op::__OP_TYPE_DIVIDER__,
        Op::FWD_INJECT_PKT, Op::REWIND,
        Op::PKT_LAT,        Op::DROP_LAT,
        Op::TIMEOUT,
    };
    const std::unordered_map<Op, std::string, OpHasher> _op_str = {
        {Op::MAIN_PROC,       "Main process"                   },
        {Op::CHECK_INVARIANT, "Check invariant"                },
        {Op::CHECK_EC,        "Check connection EC"            },
        {Op::FWD_INJECT_PKT,  "Packet injection in fwd process"},
        {Op::REWIND,          "Rewind the middlebox state"     },
        {Op::PKT_LAT,         "Packet receive latency"         },
        {Op::DROP_LAT,        "Packet drop latency"            },
        {Op::TIMEOUT,         "Actual drop timeout used"       },
    };
    std::unordered_map<Op, clock::time_point, OpHasher> _start_ts;
    std::unordered_map<Op, std::chrono::microseconds, OpHasher> _time = {
        {Op::MAIN_PROC,       {}},
        {Op::CHECK_INVARIANT, {}},
        {Op::CHECK_EC,        {}},
    };
    std::unordered_map<Op, long /* KiB */, OpHasher> _max_rss = {
        {Op::MAIN_PROC,       0},
        {Op::CHECK_INVARIANT, 0},
        {Op::CHECK_EC,        0},
    };
    std::unordered_map<Op, long /* KiB */, OpHasher> _curr_rss = {
        {Op::MAIN_PROC,       0},
        {Op::CHECK_INVARIANT, 0},
        {Op::CHECK_EC,        0},
    };
    std::unordered_map<Op, std::vector<std::chrono::microseconds>, OpHasher>
        _latencies = {
            {Op::FWD_INJECT_PKT, {}},
            {Op::REWIND,         {}},
            {Op::PKT_LAT,        {}},
            {Op::DROP_LAT,       {}},
            {Op::TIMEOUT,        {}},
    };
    /**
     * Number of packet injections when rewinding the middlebox state. -1 means
     * no rewind ever occured. 0 means the state was reset but no injection was
     * needed.
     */
    std::vector<int> _rewind_injection_count;

    Stats() = default;

    long get_peak_rss() const;
    long get_current_rss() const;
    std::pair<long, long> get_rss() const;

public:
    Stats(const Stats &) = delete;
    Stats(Stats &&) = delete;
    Stats &operator=(const Stats &) = delete;
    Stats &operator=(Stats &&) = delete;

    static Stats &get();
    const std::vector<std::chrono::microseconds> &get_pkt_latencies() const;

    void start(Op);
    void stop(Op);
    void set_rewind_injection_count(int);
    void reset();
    void log_results(Op) const;
};
