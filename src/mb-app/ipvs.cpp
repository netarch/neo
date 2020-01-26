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

    auto rules = config->get_as<std::string>("rules");

    if (!rules) {
        Logger::get().err("Missing rules");
    }

    this->rules = *rules;
}

IPVS::~IPVS()
{
}

void IPVS::init()
{
    reset();
}

void IPVS::reset()
{
    // set rules
    int fd;
    char filename[] = "/tmp/ipvs-rules.XXXXXX";
    if ((fd = mkstemp(filename)) < 0) {
        Logger::get().err(filename, errno);
    }
    if (write(fd, rules.c_str(), rules.size()) < 0) {
        Logger::get().err(filename, errno);
    }
    if (close(fd) < 0) {
        Logger::get().err(filename, errno);
    }

    if (system("ipvsadm -C")) {
        Logger::get().err("ipvsadm -C");
    }

    if (system((std::string("ipvsadm-restore ") + filename).c_str())) {
        Logger::get().err("ipvsadm-restore");
    }
    fs::remove(filename);
}

