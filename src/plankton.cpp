#include "plankton.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <set>
#include <chrono>
#include <cpptoml/cpptoml.hpp>

#include "lib/fs.hpp"
#include "lib/logger.hpp"
#include "model.h"

using namespace std::chrono;

namespace
{
bool verify_all_ECs = false;    // verify all ECs even if violation is found
bool policy_violated = false;   // true if policy is violated
std::set<int> tasks;
high_resolution_clock::time_point ec_t1, ec_t2;
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

Plankton& Plankton::get_instance()
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
    // record time usage for this EC
    ec_t2 = high_resolution_clock::now();
    auto ec_time = duration_cast<microseconds>(ec_t2 - ec_t1);

    // flush the C file stream buffers and reopen the file in C++
    fflush(NULL);
    Logger::get().reopen();
    Logger::get().info("Finished in " + std::to_string(ec_time.count())
                       + " microseconds");

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
    ec_t1 = high_resolution_clock::now();

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

    // record the total time usage
    auto total_t1 = high_resolution_clock::now();

    for (Policy *policy : policies) {
        // change working directory
        const std::string working_dir
            = fs::append(out_dir, std::to_string(policy->get_id()));
        if (!fs::exists(working_dir)) {
            fs::mkdir(working_dir);
        }
        fs::chdir(working_dir);

        // record time usage for this policy
        auto policy_t1 = high_resolution_clock::now();

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

        // record time usage for this policy
        auto policy_t2 = high_resolution_clock::now();
        auto policy_time = duration_cast<microseconds>(policy_t2 - policy_t1);
        Logger::get().info("Finished in " + std::to_string(policy_time.count())
                           + " microseconds");

        if (!verify_all_ECs && policy_violated) {
            break;
        }
    }

    // wait until all tasks are done
    while (!tasks.empty()) {
        pause();
    }

    // record the total time usage
    auto total_t2 = high_resolution_clock::now();
    auto total_time = duration_cast<microseconds>(total_t2 - total_t1);
    Logger::get().info("====================");
    Logger::get().info("Total time: " + std::to_string(total_time.count())
                       + " microseconds");

    // record the peak RSS (resident set size)
    struct rusage ru;
    if (getrusage(RUSAGE_CHILDREN, &ru) < 0) {
        fprintf(stderr, "getrusage() failed\\n");
        exit(-1);
    }
    Logger::get().info("Peak memory: " + std::to_string(ru.ru_maxrss) +
                       " kilobytes");

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

    if (state->comm != comm) {
        // communication changed, reinitialize the policy and forwarding process
        policy->init(state);
        fwd.init(state, network, policy);
    }
}

void Plankton::report(State *state)
{
    policy->report(state);
}
