#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <sys/stat.h>

#include "lib/fs.hpp"
#include "lib/logger.hpp"

namespace fs
{

void mkdir(const std::string& p)
{
    if (::mkdir(p.c_str(), 0777) < 0) {
        Logger& logger = Logger::get_instance();
        logger.err(p + ": ", errno);
        exit(EXIT_FAILURE);
    }
}

std::string realpath(const std::string& rel_p)
{
    char p[PATH_MAX];

    if (::realpath(rel_p.c_str(), p) == NULL) {
        Logger& logger = Logger::get_instance();
        logger.err(rel_p + ": ", errno);
        exit(EXIT_FAILURE);
    }

    return std::string(p);
}

std::string append(const std::string& parent, const std::string& child)
{
    std::string p(parent);

    if (!p.empty() && p.back() != '/') {
        p.push_back('/');
    }
    p.append(child);

    return p;
}

} // namespace fs
