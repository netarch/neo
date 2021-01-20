#include "mb-app/ipvs.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

void IPVS::init()
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
    if (write(rpf, "0", 1) < 0) {
        Logger::error("Failed to set rp_filter");
    }
    close(rpf);

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

    reset();
}

void IPVS::reset()
{
    // set IPVS config
    if (system("ipvsadm -C")) {
        Logger::error("ipvsadm -C");
    }
    if (system(("echo '" + config + "' | ipvsadm-restore").c_str())) {
        Logger::error("ipvsadm-restore");
    }
}
