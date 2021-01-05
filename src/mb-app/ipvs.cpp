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

    reset();
}

void IPVS::reset()
{
    // set IPVS config
    if (system("ipvsadm -C")) {
        Logger::get().err("ipvsadm -C");
    }
    if (system(("echo '" + config + "' | ipvsadm-restore").c_str())) {
        Logger::get().err("ipvsadm-restore");
    }
}
