#include "mb-app/netfilter.hpp"

#include <cstdlib>
#include <unistd.h>

#include "lib/logger.hpp"
#include "lib/net.hpp"
#include "lib/fs.hpp"

void NetFilter::init()
{
    Net::get().set_forwarding(1);
    Net::get().set_rp_filter(rp_filter);

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
