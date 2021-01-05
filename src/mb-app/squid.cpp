#include "mb-app/squid.hpp"

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"

Squid::~Squid()
{
    stop();
}

void Squid::init()
{
    // set forwarding
    int fwd = open(IPV4_FWD, O_WRONLY);
    if (fwd < 0) {
        Logger::get().err("Failed to open " IPV4_FWD, errno);
    }
    if (write(fwd, "1", 1) < 0) {
        Logger::get().err("Failed to set forwarding");
    }
    close(fwd);

    // set rp_filter
    int rpf = open(IPV4_RPF, O_WRONLY);
    if (rpf < 0) {
        Logger::get().err("Failed to open " IPV4_RPF, errno);
    }
    if (write(rpf, "0", 1) < 0) {
        Logger::get().err("Failed to set rp_filter");
    }
    close(rpf);

    // clear filtering states and rules
    if (system("iptables -F")) {
        Logger::get().err("iptables -F");
    }
    if (system("iptables -Z")) {
        Logger::get().err("iptables -Z");
    }
    if (system("iptables-restore /etc/iptables/empty.rules")) {
        Logger::get().err("iptables-restore");
    }

    //reset();

    stop();

    // set config
    int fd;
    char squid_conf[] = "/tmp/squid-conf.XXXXXX";
    if ((fd = mkstemp(squid_conf)) < 0) {
        Logger::get().err(squid_conf, errno);
    }
    if (write(fd, config.c_str(), config.size()) < 0) {
        Logger::get().err(squid_conf, errno);
    }
    if (close(fd) < 0) {
        Logger::get().err(squid_conf, errno);
    }

    // launch
    if ((pid = fork()) < 0) {
        Logger::get().err("fork squid", errno);
    } else if (pid == 0) {
        // duplicate file descriptors for logging squid output
        std::string squid_log = Logger::get().get_file() + "."
                                + std::to_string(getpid()) + ".squid";
        mode_t old_mask = umask(0111);
        int fd = open(squid_log.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
        umask(old_mask);
        if (fd < 0) {
            Logger::get().err("Failed to open " + squid_log, errno);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        fflush(stdout);
        fflush(stderr);

        execlp("squid", "squid", "-N", "-n", std::to_string(getpid()).c_str(),
               "-f", squid_conf, NULL);
        Logger::get().err("exec squid", errno);
    }

    // wait for the service to start
    usleep(500000); // 0.5 sec
}

void Squid::reset()
{
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGHUP) < 0) {
        Logger::get().err("SIGHUP squid process", errno);
    }

    // reconfigure takes less than 1s.
    // should the completeness of the reconfigure. check that by location
    // "reconfigure" in cache.0.log file
    usleep(1000000);    // 1 sec
}

void Squid::stop()
{
    if (pid == 0) {
        return;
    }

    if (kill(pid, SIGTERM) < 0) {
        Logger::get().err("SIGTERM squid process", errno);
    }

    int waited_pid;

    if ((waited_pid = wait(NULL)) < 0) {
        if (errno == ECHILD) {
            goto stopped;
        }
        Logger::get().err("Failed to wait for squid", errno);
    }

    if (waited_pid != pid) {
        Logger::get().err("Squid PID mismatch");
    }

stopped:
    pid = 0;
}
