#include "mb-app/netfilter.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

void NetFilter::init()
{
    // set forwarding
    int fwd = open(IPV4_FWD, O_WRONLY);
    if (fwd < 0) {
        Logger::error("Failed to open " IPV4_FWD, errno);
    }
    if (write(fwd, "1", 1) < 0) {
        Logger::error("Failed to set forwarding");
    }
    close(fwd);

    // set rp_filter
    int rpf = open(IPV4_RPF, O_WRONLY);
    if (rpf < 0) {
        Logger::error("Failed to open " IPV4_RPF, errno);
    }
    std::string rpf_str = std::to_string(rp_filter);
    if (write(rpf, rpf_str.c_str(), rpf_str.size()) < 0) {
        Logger::error("Failed to set rp_filter");
    }
    close(rpf);

    reset();
}

void NetFilter::reset()
{
    // set rules
    int fd;
    char filename[] = "/tmp/netfilter-rules.XXXXXX";
    if ((fd = mkstemp(filename)) < 0) {
        Logger::error(filename, errno);
    }
    if (write(fd, rules.c_str(), rules.size()) < 0) {
        Logger::error(filename, errno);
    }
    if (close(fd) < 0) {
        Logger::error(filename, errno);
    }
    /*
     * NOTE:
     * Use iptables "-w" option to wait for the shared "/run/xtables.lock".
     * Ideally we should create different mnt namespaces for each middlebox.
     * TODO: isolate (part of) the filesystem
     *
     * See:
     * https://www.spinics.net/lists/netfilter-devel/msg56960.html
     * https://www.spinics.net/lists/netdev/msg497351.html
     */
    if (system("iptables -F")) {
        Logger::error("iptables -F");
    }
    if (system("iptables -Z")) {
        Logger::error("iptables -Z");
    }
    if (system((std::string("iptables-restore ") + filename).c_str())) {
        Logger::error("iptables-restore");
    }
    fs::remove(filename);
}
