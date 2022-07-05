#include "mb-app/squid.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/net.hpp"

Squid::~Squid() {
    stop();
}

void Squid::init() {
    Logger::info("squid init");

    Net::get().set_forwarding(1);
    Net::get().set_rp_filter(0);

    Logger::info("clearing iptables filtering states and rules");
    // clear filtering states and rules
    if (system("iptables -F")) {
        Logger::error("iptables -F");
    }
    if (system("iptables -Z")) {
        Logger::error("iptables -Z");
    }
    if (system("iptables-restore /etc/iptables/empty.rules")) {
        Logger::error("iptables-restore");
    }

    // reset();
    stop();

    // set config
    int fd;
    char squid_conf[] = "/tmp/squid-conf.XXXXXX";
    if ((fd = mkstemp(squid_conf)) < 0) {
        Logger::error(squid_conf, errno);
    }
    if (write(fd, config.c_str(), config.size()) < 0) {
        Logger::error(squid_conf, errno);
    }
    if (close(fd) < 0) {
        Logger::error(squid_conf, errno);
    }
    Logger::info((std::string)"Using squid config file " + squid_conf);

    // launch
    if ((pid = fork()) < 0) {
        Logger::error("fork()", errno);
    } else if (pid == 0) {
        // duplicate file descriptors for logging squid output
        std::string squid_log =
            Logger::filename() + "." + std::to_string(getpid()) + ".squid";
        mode_t old_mask = umask(0111);
        int fd = open(squid_log.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
        umask(old_mask);
        if (fd < 0) {
            Logger::error("Failed to open " + squid_log, errno);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        fflush(stdout);
        fflush(stderr);

        Logger::info("Running command squid -N -n " + std::to_string(getpid()) + " -f " + squid_conf);
        execlp("squid", "squid", "-N", "-n", std::to_string(getpid()).c_str(),
               "-f", squid_conf, NULL);
        Logger::error("exec squid", errno);
    }

    // wait for the service to start
    usleep(500000); // 0.5 sec
}

void Squid::reset() {
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGHUP) < 0) {
        Logger::error("SIGHUP squid process", errno);
    }

    // reconfigure takes less than 1s.
    // should the completeness of the reconfigure. check that by location
    // "reconfigure" in cache.0.log file
    usleep(1000000); // 1 sec
}

void Squid::stop() {
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGTERM) < 0) {
        Logger::error("SIGTERM squid process", errno);
    }

    int waited_pid;

    if ((waited_pid = wait(NULL)) < 0) {
        if (errno == ECHILD) {
            goto stopped;
        }
        Logger::error("Failed to wait for squid", errno);
    }

    if (waited_pid != pid) {
        Logger::error("Squid PID mismatch");
    }

stopped:
    pid = 0;
}
