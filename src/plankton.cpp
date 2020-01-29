#include "plankton.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <set>
#include <cpptoml/cpptoml.hpp>

#include "stats.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"
#include "model.h"

namespace
{
bool verify_all_ECs = false;    // verify all ECs even if violation is found
bool policy_violated = false;   // true if policy is violated
std::set<int> tasks;
const int sigs[] = {SIGCHLD, SIGUSR1, SIGHUP, SIGINT, SIGQUIT, SIGTERM};

void signal_handler(int sig, siginfo_t *siginfo,
                    void *ctx __attribute__((unused)))
{
    int pid, status;

    switch (sig) {
        case SIGCHLD:
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                tasks.erase(pid);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    Logger::get().warn("Process " + std::to_string(pid)
                                       + " failed");
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
            Logger::get().warn("Policy violated in process "
                               + std::to_string(pid));
            break;
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            for (int childpid : tasks) {
                kill(childpid, sig);
            }
            while (1) {
                if (wait(nullptr) == -1 && errno == ECHILD) {
                    break;
                };
            };
            exit(128 + sig);
    }
}
} // namespace

Plankton::Plankton(): policy(nullptr)
{
}

Plankton& Plankton::get()
{
    static Plankton instance;
    return instance;
}

void Plankton::init(bool all_ECs, bool rm_out_dir, size_t dop, bool verbose,
                    const std::string& input_file,
                    const std::string& output_dir)
{
    verify_all_ECs = all_ECs;
    max_jobs = dop;
    if (rm_out_dir && fs::exists(output_dir)) {
        fs::remove(output_dir);
    }
    fs::mkdir(output_dir);
    in_file = fs::realpath(input_file);
    out_dir = fs::realpath(output_dir);
    Logger::get().set_file(fs::append(out_dir, "main.log"));
    Logger::get().set_verbose(verbose);

    auto config = cpptoml::parse_file(in_file);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");
    auto policies_config = config->get_table_array("policies");

    network = Network(nodes_config, links_config);
    policies = Policies(policies_config, network);
}

extern "C" int spin_main(int argc, const char *argv[]);

void Plankton::verify_exit(int status)
{
    Stats::get().set_verify_time();
    Stats::get().set_verify_maxrss();

    // output per proecess stats
    fflush(NULL);
    Logger::get().set_file(std::to_string(getpid()) + ".stats.csv");
    Logger::get().set_verbose(false);
    Stats::get().print_per_process_stats();

    exit(status);
}

void Plankton::verify(Policy *policy)
{
    const std::string suffix = "-t" + std::to_string(getpid()) + ".trail";
    static const char *spin_args[] = {
        "neo",
        "-E",   // suppress invalid end state errors
        "-n",   // suppress report for unreached states
        suffix.c_str(),
    };
    const std::string logfile = std::to_string(getpid()) + ".log";

    // reset signal handlers
    struct sigaction default_action;
    default_action.sa_handler = SIG_DFL;
    sigemptyset(&default_action.sa_mask);
    default_action.sa_flags = 0;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &default_action, nullptr);
    }

    // reset logger
    Logger::get().set_file(logfile);
    Logger::get().set_verbose(false);
    Logger::get().info("Policy: " + policy->to_string());

    // duplicate file descriptors
    int fd = open(logfile.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) {
        Logger::get().err("Failed to open " + logfile, errno);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // configure per process variables
    this->policy = policy;

    // record time usage for this EC
    Stats::get().set_verify_t1();

    // run SPIN verifier
    verify_exit(spin_main(sizeof(spin_args) / sizeof(char *), spin_args));
}

void Plankton::dispatch(Policy *policy)
{
    int childpid;
    if ((childpid = fork()) < 0) {
        Logger::get().err("Failed to spawn new process", errno);
    } else if (childpid == 0) {
        verify(policy);
    }

    tasks.insert(childpid);
    while (tasks.size() >= max_jobs) {
        pause();
    }
}

int Plankton::run()
{
    // register signal handlers
    struct sigaction new_action;
    new_action.sa_sigaction = signal_handler;
    sigemptyset(&new_action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&new_action.sa_mask, sigs[i]);
    }
    new_action.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &new_action, nullptr);
    }

    Stats::get().set_total_t1();

    for (Policy *policy : policies) {
        // change working directory
        const std::string working_dir
            = fs::append(out_dir, std::to_string(policy->get_id()));
        if (!fs::exists(working_dir)) {
            fs::mkdir(working_dir);
        }
        fs::chdir(working_dir);

        Stats::get().set_policy_t1();

        // verifying all combinations of ECs
        Logger::get().info("====================");
        Logger::get().info(std::to_string(policy->get_id()) + ". Verifying "
                           + policy->to_string());
        Logger::get().info("Packet ECs: " + std::to_string(policy->num_ecs()));
        while (policy->set_initial_ec()) {
            dispatch(policy);
            if (!verify_all_ECs && policy_violated) {
                break;
            }
        }
        // wait until all tasks are done
        while (!tasks.empty()) {
            pause();
        }

        Stats::get().set_policy_time();

        if (!verify_all_ECs && policy_violated) {
            break;
        }
    }

    Stats::get().set_total_time();
    Stats::get().set_total_maxrss();

    // output main process stats
    Logger::get().set_file(fs::append(out_dir, "stats.csv"));
    Logger::get().set_verbose(false);
    Stats::get().print_main_stats(network.get_nodes().size(),
                                  network.get_links().size(),
                                  policies);

    return 0;
}

void Plankton::initialize(State *state)
{
    network.init(state);
    policy->init(state);
    fwd.init(state, network, policy);
    fwd.enable();
    //sleep(7);   // DEBUG: wait for wireshark to launch
}

void Plankton::exec_step(State *state)
{
    int comm = state->comm;

    fwd.exec_step(state, network);
    policy->check_violation(state);

    if (state->comm != comm && state->choice_count > 0) {
        // communication changed
        policy->init(state);
        fwd.init(state, network, policy);
    }
}

void Plankton::report(State *state)
{
    policy->report(state);
}
