#include "plankton.hpp"

#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <list>

#include "stats.hpp"
#include "config.hpp"
#include "emulationmgr.hpp"
#include "eqclassmgr.hpp"
#include "lib/fs.hpp"
#include "lib/ip.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

static bool verify_all_ECs = false;    // verify all ECs even if a violation is found
static bool policy_violated = false;   // true when policy is violated
static std::set<int> tasks;
static const int sigs[] = {SIGCHLD, SIGUSR1, SIGHUP, SIGINT, SIGQUIT, SIGTERM};

/*
 * signal handler used by the per-connection tasks
 */
static void worker_sig_handler(int sig)
{
    int pid, status;
    switch (sig) {
        case SIGCHLD:
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    Logger::warn("Process " + std::to_string(pid) + " exited "
                                 + std::to_string(WEXITSTATUS(status)));
                }
            }
            break;
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            kill(-getpid(), sig);
            while (wait(nullptr) != -1 || errno != ECHILD);
            exit(0);
    }
}

/*
 * signal handler used by the main task and the policy tasks
 */
static void signal_handler(int sig, siginfo_t *siginfo,
                           void *ctx __attribute__((unused)))
{
    int pid, status;
    switch (sig) {
        case SIGCHLD:
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                tasks.erase(pid);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    Logger::warn("Process " + std::to_string(pid) + " exited "
                                 + std::to_string(WEXITSTATUS(status)));
                }
            }
            break;
        case SIGUSR1:   // policy violated; kill all other children
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
            // wait for all children to die out
            while (wait(nullptr) != -1 || errno != ECHILD);
            exit(0);
    }
}

Plankton::Plankton(): network(&openflow), policy(nullptr)
{
}

Plankton& Plankton::get()
{
    static Plankton instance;
    return instance;
}

void Plankton::init(bool all_ECs, bool rm_out_dir, bool latency, size_t dop,
                    int emulations, const std::string& input_file,
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

    if (emulations < 0) {
        emulations = network.get_middleboxes().size();
    }
    EmulationMgr::get().set_max_emulations(emulations);
    EmulationMgr::get().set_num_middleboxes(network.get_middleboxes().size());

    // compute policy oblivious ECs
    EqClassMgr::get().compute_policy_oblivious_ecs(network, openflow);
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

void Plankton::verify_conn()
{
    const std::string logfile = std::to_string(getpid()) + ".log";
    const std::string suffix = "-t" + std::to_string(getpid()) + ".trail";
    static const char *spin_args[] = {
        // See http://spinroot.com/spin/Man/Pan.html
        "neo",
        "-b",   // consider it error to exceed the depth limit
        "-E",   // suppress invalid end state errors
        "-n",   // suppress report for unreached states
        suffix.c_str(),
    };

    // register signal handlers
    struct sigaction action;
    action.sa_handler = worker_sig_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
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

    // record time usage for this EC
    Stats::set_ec_t1();

    // run SPIN verifier
    spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
    Logger::error("verify_exit isn't called by Spin");
}

void Plankton::verify_policy(Policy *policy)
{
    // cd to the policy working directory
    const std::string policy_wd = fs::append(out_dir, std::to_string(policy->get_id()));
    if (!fs::exists(policy_wd)) {
        fs::mkdir(policy_wd);
    }
    fs::chdir(policy_wd);

    // reset static variables
    policy_violated = false;
    tasks.clear();

    // configure per OS process variables
    this->policy = policy;

    // compute connection matrix (Cartesian product)
    policy->compute_conn_matrix();

    Logger::info("====================");
    Logger::info(std::to_string(policy->get_id()) + ". Verifying policy "
                 + policy->to_string());
    Logger::info("Connection ECs: " + std::to_string(policy->num_conn_ecs()));

    Stats::set_policy_t1();

    // fork for each combination of concurrent connections
    while ((!policy_violated || verify_all_ECs) && policy->set_conns()) {
        int childpid;
        if ((childpid = fork()) < 0) {
            Logger::error("fork()", errno);
        } else if (childpid == 0) {
            verify_conn();  // connection kid dies here
        }

        tasks.insert(childpid);
        while (tasks.size() >= max_jobs) {
            pause();
        }
    }

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
    // cd to the main working directory (output directory)
    fs::chdir(out_dir);

    // register signal handler
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

    Stats::set_total_t1();

    // fork for each policy to get their memory usage measurements
    for (Policy *policy : policies) {
        int childpid;
        if ((childpid = fork()) < 0) {
            Logger::error("fork()", errno);
        } else if (childpid == 0) {
            verify_policy(policy);  // policy kid dies here
        }

        tasks.insert(childpid);
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
    // processes
    forwarding.init(state, network);
    openflow.init(state);   // openflow has to be initialized before fib

    // policy (also initializes all connection states)
    policy->init(state, &network);

    // execution logic
    set_choice(state, 0);
    set_choice_count(state, 1);
}

void Plankton::reinit(State *state)
{
    // processes
    forwarding.init(state, network);
    openflow.init(state);   // openflow has to be initialized before fib

    // policy (also initializes all connection states)
    policy->reinit(state, &network);

    // execution logic
    set_choice(state, 0);
    set_choice_count(state, 1);
}

void Plankton::exec_step(State *state)
{
    int process_id = get_process_id(state);

    switch (process_id) {
        case pid::choose_conn: // choose the next connection
            conn_choice.exec_step(state, network);
            break;
        case pid::forwarding:
            forwarding.exec_step(state, network);
            break;
        case pid::openflow:
            openflow.exec_step(state, network);
            break;
        default:
            Logger::error("Unknown process id " + std::to_string(process_id));
    }

    int policy_result = policy->check_violation(state);
    if (policy_result & POL_REINIT_DP) {
        reinit(state);
    }

    this->check_to_switch_process(state);
}

void Plankton::check_to_switch_process(State *state) const
{
    int process_id = get_process_id(state);
    int fwd_mode = get_fwd_mode(state);
    Node *current_node = get_pkt_location(state);

    switch (process_id) {
        case pid::choose_conn:
            if (state->choice_count == 1) {
                set_process_id(state, pid::forwarding);
                state->choice_count = 1;
            }
            break;
        case pid::forwarding:
            if (fwd_mode == fwd_mode::FIRST_COLLECT ||
                    fwd_mode == fwd_mode::COLLECT_NHOPS ||
                    fwd_mode == fwd_mode::ACCEPTED ||
                    fwd_mode == fwd_mode::DROPPED) {
                if (openflow.has_updates(state, current_node)) {
                    set_process_id(state, pid::openflow);
                    state->choice_count = 2; // whether to install an update or not
                } else if (conn_choice.has_other_conns(state)) {
                    set_process_id(state, pid::choose_conn);
                    conn_choice.update_choice_count(state);
                }
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

void Plankton::report(State *state)
{
    policy->report(state);
}
