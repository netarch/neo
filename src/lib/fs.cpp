#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <sys/stat.h>
#include <ftw.h>

#include "lib/fs.hpp"
#include "lib/logger.hpp"

namespace fs
{

static Logger& logger = Logger::get_instance();

void mkdir(const std::string& p)
{
    if (::mkdir(p.c_str(), 0777) < 0) {
        logger.err(p + ": ", errno);
    }
}

bool exists(const std::string& p)
{
    struct stat buffer;
    int ret = ::stat(p.c_str(), &buffer);
    if (ret == -1) {
        if (errno == ENOENT) {
            return false;
        }
        logger.err(p + ": ", errno);
    }
    return true;
}

static int rm(const char *fpath, const struct stat *sb __attribute__((unused)),
              int typeflag, struct FTW *ftwbuf __attribute__((unused)))
{
    if (typeflag == FTW_DP) {
        if (rmdir(fpath) == -1) {
            logger.err(std::string(fpath) + ": ", errno);
        }
    } else {
        if (unlink(fpath) == -1) {
            logger.err(std::string(fpath) + ": ", errno);
        }
    }
    return 0;
}

void remove(const std::string& p)
{
    if (nftw(p.c_str(), &rm, 10000, FTW_DEPTH | FTW_PHYS) < 0) {
        logger.err("Failed to remove " + p + ": ", errno);
    }
}

std::string realpath(const std::string& rel_p)
{
    char p[PATH_MAX];

    if (::realpath(rel_p.c_str(), p) == NULL) {
        logger.err(rel_p + ": ", errno);
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
