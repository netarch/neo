#include "mb-app/netfilter.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/fs.hpp"

NetFilter::NetFilter(const std::shared_ptr<cpptoml::table>& config)
{
    auto rpf = config->get_as<int>("rp_filter");
    auto rules = config->get_as<std::string>("rules");

    if (!rpf) {
        Logger::get().err("Missing rp_filter");
    }
    if (!rules) {
        Logger::get().err("Missing rules");
    }

    if (*rpf < 0 || *rpf > 2) {
        Logger::get().err("Invalid rp_filter value: " + std::to_string(*rpf));
    }
    this->rp_filter = *rpf;
    this->rules = *rules;

    forwarding_fn = "/proc/sys/net/ipv4/conf/all/forwarding";
    rp_filter_fn = "/proc/sys/net/ipv4/conf/all/rp_filter";
}

NetFilter::~NetFilter()
{
}

void NetFilter::init()
{
    reset();
}

void NetFilter::reset()
{
    // set forwarding
    int fwd = open(forwarding_fn.c_str(), O_WRONLY);
    if (fwd < 0) {
        Logger::get().err("Failed to open " + forwarding_fn, errno);
    }
    write(fwd, "1", 1);
    close(fwd);

    // set rp_filter
    int rpf = open(rp_filter_fn.c_str(), O_WRONLY);
    if (rpf < 0) {
        Logger::get().err("Failed to open " + rp_filter_fn, errno);
    }
    std::string rpf_str = std::to_string(rp_filter);
    write(rpf, rpf_str.c_str(), rpf_str.size());
    close(rpf);

    // set rules
    int fd;
    char filename[] = "/tmp/neo-netfilter.XXXXXX";
    if ((fd = mkstemp(filename)) < 0) {
        Logger::get().err(filename, errno);
    }
    if (write(fd, rules.c_str(), rules.size()) < 0) {
        Logger::get().err(filename, errno);
    }
    if (close(fd) < 0) {
        Logger::get().err(filename, errno);
    }
    if (system("iptables -F")) {
        Logger::get().err("iptables -F");
    }
    if (system("iptables -Z")) {
        Logger::get().err("iptables -Z");
    }
    if (system((std::string("iptables-restore ") + filename).c_str())) {
        Logger::get().err("iptables-restore");
    }
    fs::remove(filename);
}
