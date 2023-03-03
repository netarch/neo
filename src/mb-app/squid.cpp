#include "mb-app/squid.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib/net.hpp"
#include "logger.hpp"

void Squid::stop() {
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGTERM) < 0) {
        logger.error("SIGTERM squid process", errno);
    }

    int waited_pid = wait(NULL);

    if (waited_pid < 0) {
        if (errno != ECHILD) {
            logger.error("Failed to wait for squid", errno);
        }
    } else if (waited_pid != pid) {
        logger.error("Squid PID mismatch");
    }

    pid = 0;
}

Squid::~Squid() {
    stop();
}

void Squid::init() {
    Net::get().set_forwarding(1);
    Net::get().set_rp_filter(0);

    // clear filtering states and rules
    if (system("iptables -F")) {
        logger.error("iptables -F");
    }
    if (system("iptables -Z")) {
        logger.error("iptables -Z");
    }
    if (system("iptables-restore /etc/iptables/empty.rules")) {
        logger.error("iptables-restore");
    }

    // reset();
    stop();

    // set config
    int fd;
    char squid_conf[] = "/tmp/squid-conf.XXXXXX";
    if ((fd = mkstemp(squid_conf)) < 0) {
        logger.error(squid_conf, errno);
    }
    if (write(fd, config.c_str(), config.size()) < 0) {
        logger.error(squid_conf, errno);
    }
    if (close(fd) < 0) {
        logger.error(squid_conf, errno);
    }

    // launch
    if ((pid = fork()) < 0) {
        logger.error("fork()", errno);
    } else if (pid == 0) {
        // duplicate file descriptors for logging squid output
        std::string squid_log =
            logger.filename() + "." + std::to_string(getpid()) + ".squid";
        mode_t old_mask = umask(0111);
        int fd = open(squid_log.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
        umask(old_mask);
        if (fd < 0) {
            logger.error("Failed to open " + squid_log, errno);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        fflush(stdout);
        fflush(stderr);

        execlp("squid", "squid", "-N", "-n", std::to_string(getpid()).c_str(),
               "-f", squid_conf, NULL);
        logger.error("exec squid", errno);
    }

    // wait for the service to start
    usleep(500000); // 0.5 sec
}

void Squid::reset() {
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGHUP) < 0) {
        logger.error("SIGHUP squid process", errno);
    }

    // reconfigure takes less than 1s.
    // should the completeness of the reconfigure. check that by location
    // "reconfigure" in cache.0.log file
    usleep(1000000); // 1 sec
}
