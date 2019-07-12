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
#include "policy/reachability.hpp"
#include "policy/stateful-reachability.hpp"
#include "policy/waypoint.hpp"

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

    Logger::get_instance().info("Loading network configurations...");

    auto config = cpptoml::parse_file(in_file);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");
    auto policies_config = config->get_table_array("policies");

    network = Network(nodes_config, links_config);

    // Load policies configurations
    if (policies_config) {
        for (auto policy_config : *policies_config) {
            std::shared_ptr<Policy> policy;
            auto type = policy_config->get_as<std::string>("type");

            if (!type) {
                Logger::get_instance().err("Missing policy type");
            }

            if (*type == "reachability") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<ReachabilityPolicy>
                          (policy_config, network));
            } else if (*type == "stateful-reachability") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<StatefulReachabilityPolicy>
                          (policy_config, network));
            } else if (*type == "waypoint") {
                policy = std::static_pointer_cast<Policy>
                         (std::make_shared<WaypointPolicy>
                          (policy_config, network));
            } else {
                Logger::get_instance().err("Unknown policy type: " + *type);
            }

            policies.push_back(std::move(policy));
        }
    }
    Logger::get_instance().info("Loaded " + std::to_string(policies.size()) +
                                " policies");

    Logger::get_instance().info("Finished loading network configurations");
}

namespace
{
std::unordered_set<int> tasks;  // active child processes (set of PIDs)
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
                Logger::get_instance().info("Joined process " +
                                            std::to_string(pid));
                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    Logger::get_instance().warn("Process " + std::to_string(pid)
                                                + " failed");
                }
            }
            break;
        case SIGUSR1:   // policy violated
            kill_all(SIGTERM);
            exit(0);
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

int Plankton::verify(const std::shared_ptr<EqClass>& ec,
                     const std::shared_ptr<Policy>& policy)
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
    Logger::get_instance().info("Start verification");
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
    this->ec = ec;
    this->policy = policy;

    // run SPIN verifier
    return spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
}

void Plankton::run()
{
    // register signal handlers
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        signal(sigs[i], signal_handler);
    }

    // compute ECs of each policy
    for (const std::shared_ptr<Policy>& policy : policies) {
        policy->compute_ecs(network);
    }

    // run verifier for each EC of each policy
    for (const std::shared_ptr<Policy>& policy : policies) {
        for (const std::shared_ptr<EqClass>& ec : policy->get_ecs()) {
            int childpid;

            if ((childpid = fork()) < 0) {
                Logger::get_instance().err("Failed to fork new processes",
                                           errno);
            } else if (childpid == 0) {
                exit(verify(ec, policy));
            }

            Logger::get_instance().info("Spawned process " +
                                        std::to_string(childpid));
            tasks.insert(childpid);
            while (tasks.size() >= max_jobs) {
                pause();
            }
        }
    }

    // wait until all tasks are done
    while (!tasks.empty()) {
        pause();
    }
}

void Plankton::update_fib()
{
    network.compute_fib(ec);
}

void Plankton::config_procs()
{
    //policy->config_procs(fwd);
}
