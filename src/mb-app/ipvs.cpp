#include "mb-app/ipvs.hpp"

#include <cstdlib>

#include "lib/net.hpp"
#include "logger.hpp"

void IPVS::init() {
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

    reset();
    Net::get().set_expire_nodest_conn(1);
}

void IPVS::reset() {
    // set IPVS config
    if (system("ipvsadm -C")) {
        logger.error("ipvsadm -C");
    }
    if (system(("echo '" + config + "' | ipvsadm-restore").c_str())) {
        logger.error("ipvsadm-restore");
    }
}
