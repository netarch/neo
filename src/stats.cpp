#include "stats.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <unistd.h>

#include "droptimeout.hpp"
#include "logger.hpp"

using namespace std;
using namespace std::chrono;

long Stats::get_peak_rss() const {
    struct rusage ru;

    if (getrusage(RUSAGE_SELF, &ru) < 0) {
        logger.error("getrusage() failed");
    }

    return ru.ru_maxrss; // KiB
}

long Stats::get_current_rss() const {
    static const string fn = "/proc/self/statm";
    static const long page_size = getpagesize() / 1024; // KiB per page
    ifstream ifs(fn);

    if (!ifs) {
        logger.error("Failed to open " + fn);
    }

    long dummy, current_page_count;
    ifs >> dummy >> current_page_count;
    return current_page_count * page_size; // KiB
}

pair<long, long> Stats::get_rss() const {
    static const string fn = "/proc/self/status";
    ifstream ifs(fn);

    if (!ifs) {
        logger.error("Failed to open " + fn);
    }

    string line;
    long maxrss = 0, currrss = 0;

    while (getline(ifs, line)) {
        if (line.starts_with("VmHWM:")) {
            maxrss = stol(line.substr(6));
        } else if (line.starts_with("VmRSS:")) {
            currrss = stol(line.substr(6));
            break;
        }
    }

    return {maxrss, currrss}; // KiB, KiB
}

Stats &Stats::get() {
    static Stats instance;
    return instance;
}

const vector<chrono::microseconds> &Stats::get_pkt_latencies() const {
    return _latencies.at(Op::PKT_LAT);
}

void Stats::start(Op op) {
    if (_start_ts.count(op) > 0) {
        logger.error("Multiple starting time point for op: " + _op_str.at(op));
    }

    _start_ts[op] = clock::now();
}

void Stats::stop(Op op) {
    auto it = _start_ts.find(op);

    if (it == _start_ts.end()) {
        logger.error("No starting time point found for op: " + _op_str.at(op));
    }

    auto duration = duration_cast<microseconds>(clock::now() - it->second);
    _start_ts.erase(it);

    if (op < Op::__OP_TYPE_DIVIDER__) {
        auto [maxrss, currrss] = this->get_rss();
        _time.at(op) = std::move(duration);
        _max_rss.at(op) = maxrss;
        _curr_rss.at(op) = currrss;
    } else if (op > Op::__OP_TYPE_DIVIDER__ && op < Op::TIMEOUT) {
        _latencies.at(op).emplace_back(std::move(duration));
        if (op == Op::PKT_LAT || op == Op::DROP_LAT) {
            set_zero_latency(op == Op::PKT_LAT ? Op::DROP_LAT : Op::PKT_LAT);
            _latencies.at(Op::TIMEOUT)
                .emplace_back(DropTimeout::get().timeout());
        }
    } else {
        logger.error("Invalid op: " + to_string(static_cast<int>(op)));
    }
}

void Stats::set_zero_latency(Op op) {
    _start_ts.erase(op);
    _latencies.at(op).emplace_back(microseconds(0));
}

void Stats::set_rewind_injection_count(int n) {
    _rewind_injection_count.push_back(n);

    for (int i = 0; i < n; ++i) {
        _latencies.at(Op::PKT_LAT).pop_back();
        _latencies.at(Op::DROP_LAT).pop_back();
        _latencies.at(Op::TIMEOUT).pop_back();
    }
}

void Stats::reset() {
    _start_ts.clear();

    for (const Op &op : _all_ops) {
        if (op < Op::__OP_TYPE_DIVIDER__) {
            _time.at(op) = microseconds{};
            _max_rss.at(op) = 0;
            _curr_rss.at(op) = 0;
        } else if (op > Op::__OP_TYPE_DIVIDER__) {
            _latencies.at(op).clear();
        }
    }

    _rewind_injection_count.clear();
}

void Stats::log_results(Op op) const {
    const auto &time = _time.at(op).count();
    const auto &max_rss = _max_rss.at(op);
    const auto &cur_rss = _curr_rss.at(op);

    if (op == Op::MAIN_PROC) {
        logger.info("====================");
        logger.info("Time: " + to_string(time) + " usec");
        logger.info("Peak memory: " + to_string(max_rss) + " KiB");
        logger.info("Current memory: " + to_string(cur_rss) + " KiB");
    } else if (op == Op::CHECK_INVARIANT) {
        const string filename = "invariant.stats.csv";
        ofstream ofs(filename);
        if (!ofs) {
            logger.error("Failed to open " + filename);
        }

        ofs << "Time (usec), Peak memory (KiB), Current memory (KiB)" << endl
            << time << ", " << max_rss << ", " << cur_rss << endl;
    } else if (op == Op::CHECK_EC) {
        const string filename = to_string(getpid()) + ".stats.csv";
        ofstream ofs(filename);
        if (!ofs) {
            logger.error("Failed to open " + filename);
        }

        ofs << "Time (usec), Peak memory (KiB), Current memory (KiB)" << endl
            << time << ", " << max_rss << ", " << cur_rss << endl;
        ofs << "Overall concretization (usec), " << "Emulation startup (usec), "
            << "Rewind (usec), " << "Emulation reset (usec), "
            << "Replay packets (usec), " << "Rewind injection count, "
            << "Packet latency (usec), " << "Drop latency (usec), "
            << "Timeout value (usec)" << endl;

        auto num_pkts = _latencies.at(Op::PKT_LAT).size();

        for (size_t i = 0; i < num_pkts; ++i) {
            if (i < _latencies.at(Op::FWD_INJECT_PKT).size()) {
                ofs << _latencies.at(Op::FWD_INJECT_PKT).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::LAUNCH_OR_GET_EMU).size()) {
                ofs << _latencies.at(Op::LAUNCH_OR_GET_EMU).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::REWIND).size()) {
                ofs << _latencies.at(Op::REWIND).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::RESET_EMU).size()) {
                ofs << _latencies.at(Op::RESET_EMU).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::REPLAY).size()) {
                ofs << _latencies.at(Op::REPLAY).at(i).count();
            }
            ofs << ", ";
            if (i < _rewind_injection_count.size()) {
                ofs << _rewind_injection_count.at(i);
            }
            ofs << ", ";
            if (i < _latencies.at(Op::PKT_LAT).size()) {
                ofs << _latencies.at(Op::PKT_LAT).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::DROP_LAT).size()) {
                ofs << _latencies.at(Op::DROP_LAT).at(i).count();
            }
            ofs << ", ";
            if (i < _latencies.at(Op::TIMEOUT).size()) {
                ofs << _latencies.at(Op::TIMEOUT).at(i).count();
            }
            ofs << endl;
        }
    } else {
        logger.error("Invalid op: " + to_string(static_cast<int>(op)));
    }
}
