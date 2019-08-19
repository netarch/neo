#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <cpptoml/cpptoml.hpp>

#include "plankton.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"

namespace
{
bool verify_all_ECs = false;    // verify all ECs even if violation is found
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
                    Logger::get_instance().warn("Process " + std::to_string(pid)
                                                + " failed");
                }
            }
            break;
        case SIGUSR1:   // policy violated; kill all the other siblings
            pid = siginfo->si_pid;
            if (!verify_all_ECs) {
                for (int childpid : tasks) {
                    if (childpid != pid) {
                        kill(childpid, SIGTERM);
                    }
                }
            }
            Logger::get_instance().warn("Policy violated in process "
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
            exit(0);
    }
}
} // namespace

Plankton::Plankton(): policy(nullptr), pre_ec(nullptr), ec(nullptr)
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
    Logger::get_instance().set_file(fs::append(out_dir, "main.log"));
    Logger::get_instance().set_verbose(verbose);

    auto config = cpptoml::parse_file(in_file);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");
    auto policies_config = config->get_table_array("policies");

    network = Network(nodes_config, links_config);
    policies = Policies(policies_config, network);
}

extern "C" int spin_main(int argc, const char *argv[]);

void Plankton::verify(Policy *policy, EqClass *pre_ec, EqClass *ec)
{
    static const char *spin_args[] = {
        "neo",
        "-E",   // suppress invalid end state errors
        "-n",   // suppress report for unreached states
        "-T",   // create trail files in read-only mode
        "-v",   // verbose
        "-x",   // don't overwrite existing trail file
    };
    const std::string logfile = fs::append(out_dir, std::to_string(getpid()) +
                                           ".log");

    // reset signal handlers
    struct sigaction default_action;
    default_action.sa_handler = SIG_DFL;
    sigemptyset(&default_action.sa_mask);
    default_action.sa_flags = 0;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &default_action, nullptr);
    }

    // reset logger
    Logger::get_instance().set_file(logfile);
    Logger::get_instance().set_verbose(false);
    Logger::get_instance().info("Policy: " + policy->to_string());

    // duplicate file descriptors
    int fd = open(logfile.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) {
        Logger::get_instance().err("Failed to open " + logfile, errno);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // configure per process variables
    this->policy = policy;
    this->pre_ec = pre_ec;
    this->ec = ec;

    // run SPIN verifier
    exit(spin_main(sizeof(spin_args) / sizeof(char *), spin_args));
}

void Plankton::dispatch(Policy *policy, EqClass *pre_ec, EqClass *ec)
{
    int childpid;
    if ((childpid = fork()) < 0) {
        Logger::get_instance().err("Failed to spawn new process", errno);
    } else if (childpid == 0) {
        verify(policy, pre_ec, ec);
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

    policies.compute_ecs(network);

    for (Policy *policy : policies) {
        Logger::get_instance().info("====================");
        Logger::get_instance().info("Verifying " + policy->to_string());
        Logger::get_instance().info("Packet ECs: "
                                    + std::to_string(policy->num_ecs()));
        for (EqClass *ec : policy->get_ecs()) {
            for (EqClass *pre_ec : policy->get_pre_ecs()) {
                dispatch(policy, pre_ec, ec);
            }
            if (policy->get_pre_ecs().empty()) {
                dispatch(policy, nullptr, ec);
            }
        }
        // wait until all tasks are done
        while (!tasks.empty()) {
            pause();
        }
    }

    return 0;
}

void Plankton::initialize(State *state)
{
    if (state->itr_ec == 0 && pre_ec) {
        Logger::get_instance().info("EC: " + pre_ec->to_string());
    } else {
        Logger::get_instance().info("EC: " + ec->to_string());
    }

    network.init(state, pre_ec, ec);
    policy->init(state);
    policy->config_procs(state, fwd);
}

void Plankton::exec_step(State *state)
{
    int old_itr_ec = state->itr_ec;

    fwd.exec_step(state);
    policy->check_violation(state);

    if (state->itr_ec != old_itr_ec && state->choice_count > 0) {
        initialize(state);
    }
}

void Plankton::report(State *state)
{
    policy->report(state);
}
