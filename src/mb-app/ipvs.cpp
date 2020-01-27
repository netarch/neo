#include "mb-app/ipvs.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

IPVS::IPVS(const std::shared_ptr<cpptoml::table>& config)
{
    timeout = std::chrono::microseconds(100);

    auto conf = config->get_as<std::string>("config");

    if (!conf) {
        Logger::get().err("Missing config");
    }

    this->config = *conf;

    forwarding_fn = "/proc/sys/net/ipv4/conf/all/forwarding";
    rp_filter_fn = "/proc/sys/net/ipv4/conf/all/rp_filter";
}

void IPVS::init()
{
    // set forwarding
    int fwd = open(forwarding_fn.c_str(), O_WRONLY);
    if (fwd < 0) {
        Logger::get().err("Failed to open " + forwarding_fn, errno);
    }
    if (write(fwd, "1", 1) < 0) {
        Logger::get().err("Failed to set forwarding");
    }
    close(fwd);

    // set rp_filter
    int rpf = open(rp_filter_fn.c_str(), O_WRONLY);
    if (rpf < 0) {
        Logger::get().err("Failed to open " + rp_filter_fn, errno);
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
