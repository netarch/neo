#include "mb-app/ipvs.hpp"

#include <cstdlib>

#include "lib/logger.hpp"
#include "lib/net.hpp"

void IPVS::init()
{
    Net::get().set_forwarding(1);
    Net::get().set_rp_filter(0);

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
