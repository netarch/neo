#include "plankton.hpp"

#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <set>

#include "stats.hpp"
#include "config.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

static bool verify_all_ECs = false;    // verify all ECs even if a violation is found
static bool policy_violated = false;   // true if policy is violated
static std::set<int> tasks;
static const int sigs[] = {SIGCHLD, SIGUSR1, SIGHUP, SIGINT, SIGQUIT, SIGTERM};

static void signal_handler(int sig, siginfo_t *siginfo,
                           void *ctx __attribute__((unused)))
{
    int pid, status;

    switch (sig) {
        case SIGCHLD:
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                tasks.erase(pid);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    Logger::warn("Process " + std::to_string(pid) + " failed");
                }
            }
            break;
        case SIGUSR1:   // policy violated; kill all the other children
            pid = siginfo->si_pid;
            policy_violated = true;
            if (!verify_all_ECs) {
                for (int childpid : tasks) {
                    if (childpid != pid) {
                        kill(childpid, SIGTERM);
                    }
                }
            }
            Logger::warn("Policy violated in process " + std::to_string(pid));
            break;
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            for (int childpid : tasks) {
                kill(childpid, sig);
            }
            while (1) { // wait for all children to die out
                if (wait(nullptr) == -1 && errno == ECHILD) {
                    break;
                };
            };
            exit(128 + sig);
    }
}

Plankton::Plankton(): policy(nullptr)
{
}

Plankton& Plankton::get()
{
    static Plankton instance;
    return instance;
}

void Plankton::init(bool all_ECs, bool rm_out_dir, size_t dop, bool latency,
                    const std::string& input_file,
                    const std::string& output_dir)
{
    verify_all_ECs = all_ECs;
    max_jobs = dop;
    if (latency) {
        Stats::enable_latency_recording();
    }
    if (rm_out_dir && fs::exists(output_dir)) {
        fs::remove(output_dir);
    }
    fs::mkdir(output_dir);
    in_file = fs::realpath(input_file);
    out_dir = fs::realpath(output_dir);
    Logger::enable_console_logging();
    Logger::enable_file_logging(fs::append(out_dir, "main.log"));

    Config::start_parsing(in_file);
    Config::parse_network(&network, in_file);
    Config::parse_openflow(&openflow, in_file, network);
    Config::parse_policies(&policies, in_file, network);
    Config::finish_parsing(in_file);
}

void Plankton::compute_policy_oblivious_ecs()
{
    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            all_ECs.add_ec(intf.first);
            owned_ECs.add_ec(intf.first);
        }
        for (const Route& route : node.second->get_rib()) {
            all_ECs.add_ec(route.get_network());
        }
    }

    for (const auto& update : openflow.get_updates()) {
        for (const Route& update_route : update.second) {
            all_ECs.add_ec(update_route.get_network());
        }
    }
}

extern "C" int spin_main(int argc, const char *argv[]);

void Plankton::verify_exit(int status)
{
    // output per EC proecess stats
    Stats::set_ec_time();
    Stats::set_ec_maxrss();
    Stats::output_ec_stats();

    exit(status);
}

static const int mb_sigs[] = {SIGCHLD};

static void mb_sig_handler(int sig)
{
    int pid, status;
    if (sig == SIGCHLD) {
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                Logger::warn("Process " + std::to_string(pid) + " failed");
            }
        }
    }
}

void Plankton::verify_ec(Policy *policy)
{
    const std::string logfile = std::to_string(getpid()) + ".log";
    const std::string suffix = "-t" + std::to_string(getpid()) + ".trail";
    static const char *spin_args[] = {
        "neo",
        "-b",   // consider it error to exceed the depth limit
        "-E",   // suppress invalid end state errors
        "-n",   // suppress report for unreached states
        suffix.c_str(),
    };

    // register signal handlers for middleboxes
    struct sigaction action;
    action.sa_handler = mb_sig_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(mb_sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, mb_sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP;
    for (size_t i = 0; i < sizeof(mb_sigs) / sizeof(int); ++i) {
        sigaction(mb_sigs[i], &action, nullptr);
    }

    // reset logger
    Logger::disable_console_logging();
    Logger::enable_file_logging(logfile);
    Logger::info("Policy: " + policy->to_string());

    // duplicate file descriptors
    int fd = open(logfile.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        Logger::error("Failed to open " + logfile, errno);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // configure per OS process variables
    this->policy = policy;

    // record time usage for this EC
    Stats::set_ec_t1();

    // run SPIN verifier
    spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
    Logger::error("verify_exit isn't called by Spin");
}

void Plankton::verify_policy(Policy *policy)
{
    Logger::info("====================");
    Logger::info(std::to_string(policy->get_id()) + ". Verifying "
                 + policy->to_string());
    Logger::info("Packet ECs: " + std::to_string(policy->num_ecs()));
    Logger::info("Communications: " + std::to_string(policy->num_comms()));

    // reset static variables
    policy_violated = false;
    tasks.clear();

    // register signal handlers
    struct sigaction action;
    action.sa_sigaction = signal_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
    }

    // cd to the policy working directory
    const std::string policy_working_dir
        = fs::append(out_dir, std::to_string(policy->get_id()));
    if (!fs::exists(policy_working_dir)) {
        fs::mkdir(policy_working_dir);
    }
    fs::chdir(policy_working_dir);

    Stats::set_policy_t1();

    // fork for each combination of ECs for concurrent execution
    while (policy->set_initial_ec()) {
        int childpid;
        if ((childpid = fork()) < 0) {
            Logger::error("fork()", errno);
        } else if (childpid == 0) {
            verify_ec(policy);
        }

        tasks.insert(childpid);
        while (tasks.size() >= max_jobs) {
            pause();
        }

        if (!verify_all_ECs && policy_violated) {
            break;
        }
    }

    // wait until all EC are verified or terminated
    while (!tasks.empty()) {
        pause();
    }

    Stats::set_policy_time();
    Stats::set_policy_maxrss();
    Stats::output_policy_stats(network.get_nodes().size(),
                               network.get_links().size(), policy);

    exit(0);
}

int Plankton::run()
{
    // compute ECs
    this->compute_policy_oblivious_ecs();
    for (Policy *policy : policies) {
        policy->compute_ecs(all_ECs, owned_ECs);
    }

    // register signal handlers
    struct sigaction action;
    action.sa_sigaction = signal_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
    }

    // cd to the main working directory (output directory)
    fs::chdir(out_dir);

    Stats::set_total_t1();

    for (Policy *policy : policies) {
        int childpid;

        // fork for each policy to get their memory usage measurements
        if ((childpid = fork()) < 0) {
            Logger::error("fork()", errno);
        } else if (childpid == 0) {
            verify_policy(policy);
        }

        tasks.insert(childpid);

        // wait until the policy finishes
        while (!tasks.empty()) {
            pause();
        }
    }

    Stats::set_total_time();
    Stats::set_total_maxrss();
    Stats::output_main_stats();

    return 0;
}


/***** functions used by the Promela network model *****/

void Plankton::initialize(State *state)
{
    network.init(state);
    policy->init(state); // this also initializes the communication settings

    set_process_id(state, pid::forwarding);
    forwarding.init(state, network, policy);
    openflow.init(state);

    //sleep(7);   // DEBUG: wait for wireshark to launch
}

void Plankton::process_switch(State *state) const
{
    int process_id = get_process_id(state);
    int fwd_mode = get_fwd_mode(state);
    Node *current_node = get_pkt_location(state);

    switch (process_id) {
        case pid::forwarding:
            if ((fwd_mode == fwd_mode::FIRST_COLLECT ||
                    fwd_mode == fwd_mode::COLLECT_NHOPS) &&
                    openflow.has_updates(state, current_node)) {
                set_process_id(state, pid::openflow);
                state->choice_count = 2; // whether to install an update or not
            }
            break;
        case pid::openflow:
            if (state->choice_count == 1) {
                set_process_id(state, pid::forwarding);
                state->choice_count = 1;
            }
            break;
        default:
            Logger::error("Unknown process id " + std::to_string(process_id));
    }
}

void Plankton::exec_step(State *state)
{
    int process_id = get_process_id(state);

    switch (process_id) {
        case pid::forwarding:
            forwarding.exec_step(state, network);
            break;
        case pid::openflow:
            openflow.exec_step(state, network);
            break;
        default:
            Logger::error("Unknown process id " + std::to_string(process_id));
    }

    this->process_switch(state);

    int policy_result = policy->check_violation(state);

    if (policy_result & POL_INIT_POL) {
        policy->init(state);
    }
    if (policy_result & POL_INIT_FWD) {
        forwarding.init(state, network, policy);
    }
    if (policy_result & POL_RESET_FWD) {
        forwarding.reset(state, network);
    }
}

void Plankton::report(State *state)
{
    policy->report(state);
}
