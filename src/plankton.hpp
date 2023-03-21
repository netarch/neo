#pragma once

#include <csignal>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/choose_conn.hpp"
#include "process/forwarding.hpp"
#include "process/openflow.hpp"

extern "C" int spin_main(int argc, const char *argv[]);

class Plankton {
private:
    // System-wide configuration
    static bool _all_ecs; // Verify all ECs
    bool _dropmon;        // Enable drop_monitor
    size_t _max_jobs;     // Max number of parallel tasks
    size_t _max_emu;      // Max number of emulations
    std::string _in_file; // Input TOML file
    std::string _out_dir; // Output directory

    // System states
    Network _network; // Network information (inc. data plane)
    std::vector<std::shared_ptr<Policy>> _policies; // Network invariants
    // TODO: policies -> invariants

    // Network processes/actors
    ChooseConnProcess _choose_conn;
    ForwardingProcess _forwarding;
    OpenflowProcess _openflow;

    // Per-policy system states
    std::shared_ptr<Policy> _policy; // Currently verified policy
    static bool _violated;           // A violation has occurred

    void verify_policy();
    void verify_conn();

    static std::unordered_set<pid_t> _tasks; // Policy or EC tasks
    static const int sigs[];
    static void inv_sig_handler(int sig, siginfo_t *siginfo, void *ctx);
    static void ec_sig_handler(int sig);
    static void kill_all_tasks(int sig, pid_t exclude_pid = 0);

    /***** functions used by the Promela network model *****/
    void check_to_switch_process() const;

private:
    friend class ConfigParser;
    Plankton();

public:
    // Disable the copy constructor and the copy assignment operator
    Plankton(const Plankton &) = delete;
    Plankton &operator=(const Plankton &) = delete;

    static Plankton &get();
    const decltype(_network) &network() const { return _network; }
    const decltype(_policies) &policies() const { return _policies; }

    void init(bool all_ecs,
              bool dropmon,
              size_t max_jobs,
              size_t max_emu,
              const std::string &input_file,
              const std::string &output_dir);
    void reset(); // Reset to as if it was just constructed
    int run();

    /***** functions used by the Promela network model *****/
    void initialize();
    void reinit(); // used by policies with correlated sub-policies
    void exec_step();
    void report();
    void verify_exit(int);
};
