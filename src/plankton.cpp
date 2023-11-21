#include "plankton.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "configparser.hpp"
#include "dropdetection.hpp"
#include "dropmon.hpp"
#include "droptimeout.hpp"
#include "droptrace.hpp"
#include "emulationmgr.hpp"
#include "eqclassmgr.hpp"
#include "logger.hpp"
#include "model-access.hpp"
#include "payloadmgr.hpp"
#include "stats.hpp"
#include "unique-storage.hpp"

using namespace std;
namespace fs = std::filesystem;

bool Plankton::_all_ecs = false;
bool Plankton::_violated = false;
bool Plankton::_terminate = false;
unordered_set<pid_t> Plankton::_tasks;
const int Plankton::sigs[] = {SIGCHLD, SIGUSR1, SIGHUP,
                              SIGINT,  SIGQUIT, SIGTERM};

Plankton::Plankton() : _max_jobs(0), _max_emu(0) {}

Plankton::~Plankton() {
    reset(/* destruct */ true);
}

Plankton &Plankton::get() {
    static Plankton instance;
    return instance;
}

void Plankton::init(bool all_ecs,
                    size_t max_jobs,
                    size_t max_emu,
                    const string &drop_method,
                    const string &input_file,
                    const string &output_dir) {
    // Initialize system-wide configuration
    this->_all_ecs = all_ecs;
    this->_max_jobs = min(max_jobs, size_t(thread::hardware_concurrency()));
    this->_max_emu = max_emu;
    this->_drop_method = drop_method;
    fs::create_directories(output_dir);
    this->_in_file = fs::canonical(input_file);
    this->_out_dir = fs::canonical(output_dir);
    logger.enable_console_logging();
    logger.enable_file_logging(fs::path(_out_dir) / "main.log");

    // Parse and load the input configurations
    ConfigParser().parse(_in_file, *this);

    // Initialize system-wide configurations
    if (this->_max_emu == 0) {
        this->_max_emu = _network.middleboxes().size();
    }

    EmulationMgr::get().max_emulations(_max_emu);
    DropTimeout::get().init();

    if (_drop_method == "dropmon") {
        drop = &DropMon::get();
    } else if (_drop_method == "ebpf") {
        drop = &DropTrace::get();
    } else {
        drop = nullptr;
    }

    if (drop) {
        drop->init();
    }

    // Compute initial ECs (oblivious to the invariants)
    auto &ec_mgr = EqClassMgr::get();
    ec_mgr.compute_initial_ecs(_network, _openflow);
    logger.info("Initial ECs: " + to_string(ec_mgr.all_ecs().size()));
    logger.info("Initial ports: " + to_string(ec_mgr.ports().size()));
}

// Reset to as if it was just constructed
void Plankton::reset(bool destruct) {
    this->_all_ecs = false;
    this->_max_jobs = 0;
    this->_max_emu = 0;
    this->_in_file.clear();
    this->_out_dir.clear();
    this->_network.reset();
    this->_invs.clear();
    this->_choose_conn.reset();
    this->_forwarding.reset();
    this->_openflow.reset();
    this->_inv.reset();
    this->_violated = false;
    this->_terminate = false;
    this->kill_all_tasks(SIGKILL);
    this->_tasks.clear();

    // Reset system-wide configurations
    // During program destruction, these singletons will get destroyed
    // automatically, so we don't reset them to avoid use after free.
    if (!destruct) {
        EmulationMgr::get().reset();
        EqClassMgr::get().reset();
        DropTimeout::get().reset();
        DropMon::get().stop();
        DropMon::get().teardown();
        DropTrace::get().teardown();
        PayloadMgr::get().reset();
        model.reset();
        storage.reset();
        drop = nullptr;
        _STATS_RESET();
    }
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

    DropMon::get().start(); // Start kernel drop_monitor (if enabled)

    _STATS_START(Stats::Op::MAIN_PROC);

    // Fork for each invariant to get their memory usage measurements
    for (const auto &inv : _invs) {
        pid_t childpid;

        if ((childpid = fork()) < 0) {
            logger.error("fork()", errno);
        } else if (childpid == 0) {
            this->_inv = inv;
            verify_invariant();
            exit(0);
        }

        this->_tasks.insert(childpid);

        while (!this->_tasks.empty() && !this->_terminate) {
            pause();
        }

        if (this->_terminate) {
            break;
        }
    }

    _STATS_STOP(Stats::Op::MAIN_PROC);
    _STATS_LOGRESULTS(Stats::Op::MAIN_PROC);

    DropMon::get().stop(); // Stop kernel drop_monitor (if enabled)
    return 0;
}

/**
 * This signal handler is used by both the main process and the invariant
 * processes.
 */
void Plankton::inv_sig_handler(int sig,
                               siginfo_t *siginfo,
                               [[maybe_unused]] void *ctx) {
    pid_t pid;

    switch (sig) {
    case SIGCHLD: {
        int status, rc;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            _tasks.erase(pid);

            if (WIFEXITED(status) && (rc = WEXITSTATUS(status)) != 0) {
                // A task exited abnormally. Halt all verification tasks.
                logger.warn("Process " + to_string(pid) + " exited " +
                            to_string(rc));
                kill_all_tasks(SIGTERM);
                _terminate = true;
            }
        }
        break;
    }
    case SIGUSR1: {
        // Invariant violated. Stop all remaining tasks
        pid = siginfo->si_pid;
        logger.warn("Invariant violated in process " + to_string(pid));
        _violated = true;

        if (!_all_ecs) {
            kill_all_tasks(SIGTERM, /* exclude */ pid);
        }

        break;
    }
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM: {
        logger.warn("Killed");
        kill_all_tasks(sig);
        _terminate = true;
        break;
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

void Plankton::verify_invariant() {
    // Change to the invariant output directory
    const auto inv_dir = fs::path(_out_dir) / to_string(_inv->id());
    fs::create_directory(inv_dir);
    fs::current_path(inv_dir);

    // Initialize per-invariant system states
    this->_violated = false;
    this->_tasks.clear();

    // Compute connection matrix (Cartesian product)
    this->_inv->compute_conn_matrix();

    // Update latency estimate
    int nprocs = min(this->_inv->num_conn_ecs(), _max_jobs);
    DropTimeout::get().adjust_latency_estimate_by_nprocs(nprocs);

    logger.info("====================");
    logger.info(to_string(_inv->id()) + ". Verifying invariant " +
                _inv->to_string());
    logger.info("Connection ECs: " + to_string(_inv->num_conn_ecs()));

    _STATS_START(Stats::Op::CHECK_INVARIANT);

    // Fork for each combination of concurrent connections
    while ((!_violated || _all_ecs) && !_terminate && _inv->set_conns()) {
        pid_t childpid;

        if ((childpid = fork()) < 0) {
            logger.error("fork()", errno);
        } else if (childpid == 0) {
            verify_conn();
            exit(0);
        }

        _tasks.insert(childpid);

        while (_tasks.size() >= _max_jobs && !_terminate) {
            pause();
        }
    }

    while (!_tasks.empty() && !_terminate) {
        pause();
    }

    _STATS_STOP(Stats::Op::CHECK_INVARIANT);
    _STATS_LOGRESULTS(Stats::Op::CHECK_INVARIANT);
}

void Plankton::verify_conn() {
    // Change to the invariant output directory
    const auto inv_dir = fs::path(_out_dir) / to_string(_inv->id());
    fs::create_directory(inv_dir);
    fs::current_path(inv_dir);

    // Reset logger
    const auto log_path = inv_dir / (to_string(getpid()) + ".log");
    logger.disable_console_logging();
    logger.disable_file_logging();
    logger.enable_file_logging(log_path);
    logger.info("Invariant: " + _inv->to_string());

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

    // Open and load the BPF program (if enabled)
    DropTrace::get().start();

    // Run SPIN verifier
    const string trail_suffix = "-t" + to_string(getpid()) + ".trail";
    const char *spin_args[] = {
        // See http://spinroot.com/spin/Man/Pan.html
        "neo",
        "-b", // consider it error to exceed the depth limit
        "-E", // suppress invalid end state errors
        "-n", // suppress report for unreached states
        trail_suffix.c_str(),
    };
    _STATS_START(Stats::Op::CHECK_EC);
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
                logger.warn("Process " + to_string(pid) + " exited " +
                            to_string(rc));
                kill(-getpid(), SIGTERM);
                _terminate = true;
            }
        }
        break;
    }
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM: {
        logger.warn("Killed");
        kill(-getpid(), sig);
        while (wait(nullptr) != -1 || errno != ECHILD)
            ;
        _terminate = true;
        break;
    }
    }
}

/***** functions used by the Promela network model *****/

void Plankton::initialize() {
    if (_terminate) {
        verify_exit(0);
    }

    model.init(&_network, &_openflow);

    // processes
    _forwarding.init(_network);
    _openflow.init(); // openflow has to be initialized before fib

    // invariant (also initializes all connection states)
    _inv->init();

    // control state
    model.set_process_id(pid::forwarding);
    model.set_choice(0);
    model.set_choice_count(1);
}

void Plankton::reinit() {
    if (_terminate) {
        verify_exit(0);
    }

    // processes
    _forwarding.init(_network);
    _openflow.init(); // openflow has to be initialized before fib

    // invariant (also initializes all connection states)
    _inv->reinit();

    // control state
    model.set_process_id(pid::forwarding);
    model.set_choice(0);
    model.set_choice_count(1);
}

void Plankton::exec_step() {
    if (_terminate) {
        verify_exit(0);
    }

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

    int inv_res = _inv->check_violation();
    if (inv_res & POL_REINIT_DP) {
        reinit();
    }
}

void Plankton::check_to_switch_process() const {
    if (_terminate) {
        verify_exit(0);
    }

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

void Plankton::report() const {
    if (_terminate) {
        verify_exit(0);
    }

    _inv->report();
}

void Plankton::verify_exit(int status) const {
    // Output per EC process stats
    _STATS_STOP(Stats::Op::CHECK_EC);
    _STATS_LOGRESULTS(Stats::Op::CHECK_EC);

    // Remove the BPF program (if enabled)
    DropTrace::get().stop();

    exit(status);
}
