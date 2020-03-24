#include "lib/fs.hpp"

#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <fstream>

#include "lib/logger.hpp"

namespace fs
{

void chdir(const std::string& wd)
{
    if (::chdir(wd.c_str()) < 0) {
        Logger::get().err(wd, errno);
    }
}

void mkdir(const std::string& p)
{
    if (::mkdir(p.c_str(), 0777) < 0) {
        Logger::get().err(p, errno);
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
        Logger::get().err(p, errno);
    }
    return true;
}

static int rm(const char *fpath, const struct stat *sb __attribute__((unused)),
              int typeflag, struct FTW *ftwbuf __attribute__((unused)))
{
    if (typeflag == FTW_D || typeflag == FTW_DNR || typeflag == FTW_DP) {
        if (rmdir(fpath) == -1) {
            return -1;
        }
    } else {
        if (unlink(fpath) == -1) {
            return -1;
        }
    }
    return 0;
}

void remove(const std::string& p)
{
    if (nftw(p.c_str(), &rm, 10000, FTW_DEPTH | FTW_PHYS) < 0) {
        Logger::get().err(p, errno);
    }
}

void copy(const std::string& src, const std::string& dst)
{
    std::string src_path = realpath(src);
    std::string dst_path = realpath(dst);

    if (src_path == dst_path) {
        return;
    }

    std::ifstream src_f(src_path, std::ios::binary);
    std::ofstream dst_f(dst_path, std::ios::binary);

    dst_f << src_f.rdbuf();

    src_f.close();
    dst_f.close();
}

bool is_regular(const std::string& path)
{
    struct stat buffer;
    int ret = stat(path.c_str(), &buffer);
    if (ret == -1) {
        Logger::get().err(path, errno);
    }
    return S_ISREG(buffer.st_mode);
}

std::string getcwd()
{
    char p[PATH_MAX];

    if (::getcwd(p, sizeof(p)) == NULL) {
        Logger::get().err("getcwd", errno);
    }

    return std::string(p);
}

std::string realpath(const std::string& rel_p)
{
    char p[PATH_MAX];

    if (::realpath(rel_p.c_str(), p) == NULL) {
        Logger::get().err(rel_p, errno);
    }

    return std::string(p);
}

std::string append(const std::string& parent, const std::string& child)
{
    std::string p(parent);

    if (!p.empty() && p.back() != '/' &&
            !child.empty() && child.front() != '/') {
        p.push_back('/');
    }
    p.append(child);

    return p;
}

std::shared_ptr<cpptoml::table> get_toml_config(
    const std::string& content)
{
    int fd;
    char filename[] = "/tmp/neo-config.XXXXXX";
    std::shared_ptr<cpptoml::table> config;

    if ((fd = mkstemp(filename)) < 0) {
        Logger::get().err(filename, errno);
    }
    if (write(fd, content.c_str(), content.size()) < 0) {
        Logger::get().err(filename, errno);
    }
    if (close(fd) < 0) {
        Logger::get().err(filename, errno);
    }
    config = cpptoml::parse_file(filename);
    remove(filename);
    return config;
}

} // namespace fs
