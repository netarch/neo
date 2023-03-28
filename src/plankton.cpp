#include "plankton.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "configparser.hpp"
#include "droptimeout.hpp"
#include "emulationmgr.hpp"
#include "eqclassmgr.hpp"
#include "logger.hpp"
#include "model-access.hpp"
#include "stats.hpp"

using namespace std;
namespace fs = std::filesystem;

bool Plankton::_all_ecs = false;
bool Plankton::_violated = false;
unordered_set<pid_t> Plankton::_tasks;
const int Plankton::sigs[] = {SIGCHLD, SIGUSR1, SIGHUP,
                              SIGINT,  SIGQUIT, SIGTERM};

Plankton::Plankton() : _dropmon(false), _max_jobs(0), _max_emu(0) {}

Plankton &Plankton::get() {
    static Plankton instance;
    return instance;
}

void Plankton::init(bool all_ecs,
                    bool dropmon,
                    size_t max_jobs,
                    size_t max_emu,
                    const string &input_file,
                    const string &output_dir) {
    // Initialize system-wide configuration
    this->_all_ecs = all_ecs;
    this->_dropmon = dropmon;
    this->_max_jobs = min(max_jobs, size_t(thread::hardware_concurrency()));
    this->_max_emu = max_emu;
    fs::create_directories(output_dir);
    this->_in_file = fs::canonical(input_file);
    this->_out_dir = fs::canonical(output_dir);
    logger.enable_console_logging();
    logger.enable_file_logging(fs::path(_out_dir) / "main.log");

    // Set the initial system state based on input configuration
    ConfigParser().parse(_in_file, *this);
    if (this->_max_emu == 0) {
        this->_max_emu = _network.get_middleboxes().size();
    }
    EmulationMgr::get().max_emulations(_max_emu);
    // DropMon::get().init(dropmon);

    // Compute initial ECs (oblivious to the policies)
    auto &ec_mgr = EqClassMgr::get();
    ec_mgr.compute_initial_ecs(_network, _openflow);
    logger.info("Initial ECs: " + to_string(ec_mgr.all_ecs().size()));
    logger.info("Initial ports: " + to_string(ec_mgr.ports().size()));
}

// Reset to as if it was just constructed
void Plankton::reset() {
    this->_all_ecs = false;
    this->_dropmon = false;
    this->_max_jobs = 0;
    this->_max_emu = 0;
    this->_in_file.clear();
    this->_out_dir.clear();
    this->_network.reset();
    this->_policies.clear();
    this->_choose_conn.reset();
    this->_forwarding.reset();
    this->_openflow.reset();
    this->_policy.reset();
    this->_violated = false;
    this->kill_all_tasks(SIGKILL);
    this->_tasks.clear();
}

int Plankton::run() {
    // Register signal handler
    struct sigaction action;
    action.sa_sigaction = inv_sig_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
    }

    // DropMon::get().start();
    _STATS_START(Stats::Op::MAIN_PROC);

    // Fork for each policy to get their memory usage measurements
    for (const auto &policy : _policies) {
        pid_t childpid;

        if ((childpid = fork()) < 0) {
            logger.error("fork()", errno);
        } else if (childpid == 0) {
            this->_policy = policy;
            verify_policy();
            exit(0);
        }

        this->_tasks.insert(childpid);

        while (!this->_tasks.empty()) {
            pause();
        }
    }

    _STATS_STOP(Stats::Op::MAIN_PROC);
    _STATS_LOGRESULTS(Stats::Op::MAIN_PROC);
    // DropMon::get().stop();

    return 0;
}

/**
 * This signal handler is used by both the main process and the invariant/policy
 * processes.
 */
void Plankton::inv_sig_handler(int sig,
                               siginfo_t *siginfo,
                               void *ctx __attribute__((unused))) {
    pid_t pid;

    switch (sig) {
    case SIGCHLD: {
        int status, rc;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            _tasks.erase(pid);

            if (WIFEXITED(status) && (rc = WEXITSTATUS(status)) != 0) {
                // A task exited abnormally. Halt all verification tasks.
                kill_all_tasks(SIGTERM);
                // DropMon::get().stop();
                logger.error("Process " + to_string(pid) + " exited " +
                             to_string(rc));
            }
        }
        break;
    }
    case SIGUSR1: {
        // Policy violated. Stop all remaining tasks
        pid = siginfo->si_pid;
        _violated = true;

        if (!_all_ecs) {
            kill_all_tasks(SIGTERM, /* exclude */ pid);
        }

        logger.warn("Policy violated in process " + to_string(pid));
        break;
    }
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM: {
        kill_all_tasks(sig);
        // DropMon::get().stop();
        exit(0);
    }
    }
}

void Plankton::kill_all_tasks(int sig, pid_t exclude_pid) {
    if (exclude_pid) {
        _tasks.erase(exclude_pid);
    }

    for (pid_t pid : _tasks) {
        kill(pid, sig);
    }

    while (wait(nullptr) != -1 || errno != ECHILD)
        ;

    _tasks.clear();
}

void Plankton::verify_policy() {
    // Change to the invariant output directory
    const auto inv_dir = fs::path(_out_dir) / to_string(_policy->get_id());
    fs::create_directory(inv_dir);
    fs::current_path(inv_dir);

    // Initialize per-policy system states
    this->_violated = false;
    this->_tasks.clear();

    // Compute connection matrix (Cartesian product)
    this->_policy->compute_conn_matrix();

    // Update latency estimate
    int nprocs = min(this->_policy->num_conn_ecs(), _max_jobs);
    DropTimeout::get().adjust_latency_estimate_by_nprocs(nprocs);

    logger.info("====================");
    logger.info(to_string(_policy->get_id()) + ". Verifying policy " +
                _policy->to_string());
    logger.info("Connection ECs: " + to_string(_policy->num_conn_ecs()));

    _STATS_START(Stats::Op::CHECK_INVARIANT);

    // Fork for each combination of concurrent connections
    while ((!_violated || _all_ecs) && _policy->set_conns()) {
        pid_t childpid;

        if ((childpid = fork()) < 0) {
            logger.error("fork()", errno);
        } else if (childpid == 0) {
            verify_conn();
            exit(0);
        }

        _tasks.insert(childpid);

        while (_tasks.size() >= _max_jobs) {
            pause();
        }
    }

    while (!_tasks.empty()) {
        pause();
    }

    _STATS_STOP(Stats::Op::CHECK_INVARIANT);
    _STATS_LOGRESULTS(Stats::Op::CHECK_INVARIANT);
}

void Plankton::verify_conn() {
    // Change to the invariant output directory
    const auto inv_dir = fs::path(_out_dir) / to_string(_policy->get_id());
    fs::create_directory(inv_dir);
    fs::current_path(inv_dir);

    // Reset logger
    const auto log_path = inv_dir / (to_string(getpid()) + ".log");
    logger.disable_console_logging();
    logger.disable_file_logging();
    logger.enable_file_logging(log_path);
    logger.info("Policy: " + _policy->to_string());

    // Duplicate file descriptors for SPIN stdout/stderr
    int fd = open(log_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        logger.error("Failed to open " + log_path.string(), errno);
    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    // Register signal handlers
    struct sigaction action;
    action.sa_handler = ec_sig_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    action.sa_flags = SA_NOCLDSTOP;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
    }

    // Spawn a dropmon thread (will be joined and disconnected on exit)
    // DropMon::get().connect();

    _STATS_START(Stats::Op::CHECK_EC);

    // run SPIN verifier
    const string trail_suffix = "-t" + to_string(getpid()) + ".trail";
    const char *spin_args[] = {
        // See http://spinroot.com/spin/Man/Pan.html
        "neo",
        "-b", // consider it error to exceed the depth limit
        "-E", // suppress invalid end state errors
        "-n", // suppress report for unreached states
        trail_suffix.c_str(),
    };
    spin_main(sizeof(spin_args) / sizeof(char *), spin_args);
    logger.error("verify_exit isn't called by Spin");
}

/**
 * This signal handler is used by the connection EC processes.
 */
void Plankton::ec_sig_handler(int sig) {
    // SIGUSR1 is used for unblocking the emulation listener thread, so we
    // capture the signal but do nothing for it.
    switch (sig) {
    case SIGCHLD: {
        pid_t pid;
        int status, rc;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            if (WIFEXITED(status) && (rc = WEXITSTATUS(status)) != 0) {
                logger.error("Process " + to_string(pid) + " exited " +
                             to_string(rc));
            }
        }
        break;
    }
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM: {
        kill(-getpid(), sig);
        while (wait(nullptr) != -1 || errno != ECHILD)
            ;
        logger.info("Killed");
        exit(0);
    }
    }
}

/***** functions used by the Promela network model *****/

void Plankton::initialize() {
    model.init(&_network, &_openflow);

    // processes
    _forwarding.init(_network);
    _openflow.init(); // openflow has to be initialized before fib

    // policy (also initializes all connection states)
    _policy->init();

    // control state
    model.set_process_id(pid::forwarding);
    model.set_choice(0);
    model.set_choice_count(1);
}

void Plankton::reinit() {
    // processes
    _forwarding.init(_network);
    _openflow.init(); // openflow has to be initialized before fib

    // policy (also initializes all connection states)
    _policy->reinit();

    // control state
    model.set_process_id(pid::forwarding);
    model.set_choice(0);
    model.set_choice_count(1);
}

void Plankton::exec_step() {
    int process_id = model.get_process_id();

    switch (process_id) {
    case pid::choose_conn: // choose the next connection
        _choose_conn.exec_step();
        break;
    case pid::forwarding:
        _forwarding.exec_step();
        break;
    case pid::openflow:
        _openflow.exec_step();
        break;
    default:
        logger.error("Unknown process id " + to_string(process_id));
    }

    this->check_to_switch_process();

    int policy_result = _policy->check_violation();
    if (policy_result & POL_REINIT_DP) {
        reinit();
    }
}

void Plankton::check_to_switch_process() const {
    if (model.get_executable() != 2) {
        /**
         * 2: executable, not entering a middlebox
         * 1: executable, about to enter a middlebox
         * 0: not executable (missing packet or terminated)
         */
        model.set_process_id(pid::choose_conn);
        _choose_conn.update_choice_count();
        return;
    }

    int process_id = model.get_process_id();
    int fwd_mode = model.get_fwd_mode();
    Node *current_node = model.get_pkt_location();

    switch (process_id) {
    case pid::choose_conn:
        model.set_process_id(pid::forwarding);
        break;
    case pid::forwarding:
        if ((fwd_mode == fwd_mode::COLLECT_NHOPS ||
             fwd_mode == fwd_mode::FIRST_COLLECT) &&
            _openflow.has_updates(current_node)) {
            model.set_process_id(pid::openflow);
            model.set_choice_count(2); // whether to install an update or not
        }
        break;
    case pid::openflow:
        if (model.get_choice_count() == 1) {
            model.set_process_id(pid::forwarding);
        }
        break;
    default:
        logger.error("Unknown process id " + to_string(process_id));
    }
}

void Plankton::report() {
    _policy->report();
}

void Plankton::verify_exit(int status) {
    // output per EC process stats
    _STATS_STOP(Stats::Op::CHECK_EC);
    _STATS_LOGRESULTS(Stats::Op::CHECK_EC);
    exit(status);
}
