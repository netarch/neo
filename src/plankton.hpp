#pragma once

#include <csignal>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "invariant/invariant.hpp"
#include "network.hpp"
#include "process/choose_conn.hpp"
#include "process/forwarding.hpp"
#include "process/openflow.hpp"

extern "C" int spin_main(int argc, const char *argv[]);

class Plankton {
private:
    // System-wide configuration
    static bool _all_ecs;       // Verify all ECs
    static bool _parallel_invs; // Allow verifying invariants in parallel
    size_t _max_jobs;           // Max number of parallel tasks
    size_t _max_emu;            // Max number of emulations
    std::string _drop_method;   // Drop detection method
    std::string _in_file;       // Input TOML file
    std::string _out_dir;       // Output directory

    // System states
    Network _network; // Network information (inc. data plane)
    std::vector<std::shared_ptr<Invariant>> _invs; // Network invariants

    // Network processes/actors
    ChooseConnProcess _choose_conn;
    ForwardingProcess _forwarding;
    OpenflowProcess _openflow;

    // Per-invariant system states
    std::shared_ptr<Invariant> _inv; // Currently verified invariant
    static bool _violated;           // A violation has occurred

    void verify_invariant();
    void verify_conn();

    static bool _terminate;                  // Terminate the entire program
    static std::unordered_set<pid_t> _tasks; // Invariant or EC tasks
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
    // Disable the copy/move constructors and the assignment operators
    Plankton(const Plankton &) = delete;
    Plankton(Plankton &&) = delete;
    Plankton &operator=(const Plankton &) = delete;
    Plankton &operator=(Plankton &&) = delete;
    ~Plankton();

    static Plankton &get();
    const decltype(_network) &network() const { return _network; }
    const decltype(_invs) &invariants() const { return _invs; }

    void init(bool all_ecs,
              bool parallel_invs,
              size_t max_jobs,
              size_t max_emu,
              const std::string &drop_method,
              const std::string &input_file,
              const std::string &output_dir);
    void reset(bool destruct = false); // Reset as if it was just constructed
    int run();

    /***** functions used by the Promela network model *****/
    void initialize();
    void reinit(); // used by invariants with correlated sub-invariants
    void exec_step();
    void report() const;
    void verify_exit(int) const;
};
