#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <cpptoml/cpptoml.hpp>

#include "plankton.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"

Plankton::Plankton(): policy(nullptr), pre_ec(nullptr), ec(nullptr)
{
}

Plankton& Plankton::get_instance()
{
    static Plankton instance;
    return instance;
}

void Plankton::init(bool verbose, bool rm_out_dir, size_t dop,
                    const std::string& input_file,
                    const std::string& output_dir)
{
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

namespace
{
bool violated;  // whether the current verifying policy is violated
std::unordered_set<int> tasks;
const int sigs[] = {SIGCHLD, SIGUSR1, SIGINT, SIGQUIT, SIGTERM, SIGKILL};

inline void kill_all(int sig)
{
    for (int childpid : tasks) {
        kill(childpid, sig);
    }
}

void signal_handler(int sig)
{
    switch (sig) {
        case SIGCHLD:
            int pid, status;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                tasks.erase(pid);
                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    Logger::get_instance().warn("Process " + std::to_string(pid)
                                                + " failed");
                }
            }
            break;
        case SIGUSR1:   // policy violated
            violated = true;
            kill_all(SIGTERM);
            break;
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGKILL:
            kill_all(sig);
            exit(0);
    }
}
} // namespace

extern "C" int spin_main(int argc, const char *argv[]);

void Plankton::verify(Policy *policy, EqClass *pre_ec, EqClass *ec)
{
    static const char spin_param0[] = "neo";
    static const char spin_param1[] = "-m100000";
    static const char *spin_args[] = {spin_param0, spin_param1};
    const std::string logfile = fs::append(out_dir, std::to_string(getpid()) +
                                           ".log");

    // reset signal handlers
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        signal(sigs[i], SIG_DFL);
    }

    // reset logger
    Logger::get_instance().set_file(logfile);
    Logger::get_instance().set_verbose(false);
    Logger::get_instance().info("EC: " + ec->to_string());
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
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        signal(sigs[i], signal_handler);
    }

    for (Policy *policy : policies) {
        Logger::get_instance().info("Verifying " + policy->to_string());
        policy->compute_ecs(network);
        Logger::get_instance().info("Packet ECs: "
                                    + std::to_string(policy->num_ecs()));
        violated = false;

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

        if (violated) {
            Logger::get_instance().info("Policy violated");
        } else {
            Logger::get_instance().info("Policy holds");
        }
    }

    return 0;
}

void Plankton::initialize(State *state)
{
    if (state->itr_ec == 0) {
        if (pre_ec) {
            network.fib_init(state, pre_ec);
        } else {
            network.fib_init(state, ec);
        }
    } else {
        network.fib_init(state, ec);
    }

    // TODO: initialize update history

    policy->procs_init(state, fwd);
    //Logger::get_instance().info("Initialization finished");
}

void Plankton::execute(State *state)
{
    fwd.exec_step(state);
    policy->check_violation(state);

    if (state->choice_count == 0 && state->itr_ec == 0 && pre_ec) {
        state->itr_ec = 1;
        initialize(state);
    }
}

void Plankton::report(State *state)
{
    policy->report(state);
}
