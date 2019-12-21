#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <ios>
#include <cstdio>
#include <fcntl.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <ctime>
#include "mb-app/cache-proxy.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"

/**
const std::string log_dir = fs::append(curr_dir, "examples/configs/squid/log/");
const std::string config_dir = fs::append(curr_dir, "examples/config/squid/");
const std::string cache_log = fs::append(log_dir, "cache.log");         // debug info for server
const std::string access_log = fs::append(log_dir, "access.log");       // cache hit status
const std::string cache_store_log = fs::append(log_dir, "store.log");   // cache object status
**/
static int PROXY_CONFIG = 0;
static int NAT_CONFIG = 1;
const std::string config_dir = "examples/configs/squid/";
void get_timestamp(char *buffer, int len);
static inline void ltrim(std::string& str);
// current workflow:
// 1, get config in toml
// 2, generate configuration file
// 3, build up cmd & nat_cmd & proxy_config_path & nat_config_path
CacheProxy::CacheProxy( const std::shared_ptr<cpptoml::table>& node)
{
    server_pid = 0;
    curr_dir = fs::get_cwd();
    Logger::get().info("curr_dir " + curr_dir);
    proxy_path = "/usr/sbin/squid";
    std::string squid("squid-proxy");

    char buff[100];
    get_timestamp(buff, 100);
    std::string proxy_config_path;
    std::string log_dir = fs::append(curr_dir, "examples/configs/squid/log/");
    if (!fs::exists(log_dir)) {
        fs::mkdir(log_dir);
    }
    if (node) {
        auto app = node->get_as<std::string>("app");
        if (app && !squid.compare(*app)) {
            auto config = node->get_as<std::string>("config");
            auto name = node->get_as<std::string>("name");
            auto ptr = node->get_as<std::string>("prefer_direct");
            proxy_config_path = fs::append(curr_dir, config_dir + "proxy_config-" + *name + '-' + std::string(buff));
            prefer_direct = (std::string) (*ptr);
            std::string raw_config = *config;
            Logger::get().info("Prefer_directive directive: " + prefer_direct);

            // split the file into two part, one as nat configuration, one as squid configuration
            size_t nat_index = raw_config.find("*nat");
            size_t proxy_index = (nat_index == std::string::npos) ? raw_config.find("*squid") : raw_config.find("*squid", nat_index);
            std::string nat_config = raw_config.substr(nat_index + strlen("*nat"), proxy_index);
            std::string proxy_config = raw_config.substr(proxy_index + strlen("*squid"));

            if (!generate_config_file(proxy_config, proxy_config_path, PROXY_CONFIG)) {
                Logger::get().info("proxy config file generated, file path: " + proxy_config_path);
            } else {
                exit(EXIT_FAILURE);
            }
            if (nat_config.length() > 0) {
                // only get the nat setting if the config contains nat config
                std::string nat_config_path = fs::append(curr_dir, config_dir + "nat_config-" + *name + "-" + std::string(buff));
                if (!generate_config_file(nat_config, nat_config_path, NAT_CONFIG)) {
                    Logger::get().info("nat config file generated, file path: " + nat_config_path);
                    // iptable_cmd = "iptable-restore " + nat_config_path;
                } else {
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            Logger::get().err("it is not a squid node");
        }
    } else {
        Logger::get().err("Config entry for this node is empty, please check the .toml file for the experiment");
        // exit(EXIT_FAILURE); // terminate the calling process
    }
    cmd = proxy_path + " -N" + " -f " + proxy_config_path; // -C to avoid fatal signals during running
    Logger::get().info("Cache Proxy is created..");

    // reserve previous log files
    /**
    if (fs::exists(log_dir)) {
        if (fs::exists(cache_log)) {
            std::rename(cache_log.c_str(), (cache_log + "-" + std::string(buff)).c_str());
        }
        if (fs::exists(access_log)) {
            std::rename(access_log.c_str(), (access_log + "-" + std::string(buff)).c_str());
        }
        if (fs::exists(cache_store_log)) {
            std::rename(cache_store_log.c_str(), (cache_store_log + "-" + std::string(buff)).c_str());
        }
    } else {
        fs::mkdir(log_dir);
    }
    **/
}

CacheProxy::~CacheProxy()
{
    shutnclean();
    Logger::get().info("Cache Proxy is destroyed..");
}

/** clean cache storage and launch server in child process**/
// init workflow
// 0, clean previous launched proxy if existed
// 1, fork process to launch the squid program
// 2, chekc that server is launched with pid file
void CacheProxy::init()
{
    Logger::get().info("Start to init Cache Proxy middle box");
    if (server_pid > 0) {
        shutnclean();
    }

    /**
    // first launch, set the middlebox environment first if intercet is used
    // iptable configuration file loaded with `iptable-restore`
    if (first_launch && !iptable_cmd.empty())
    {
        int status = system(iptable_cmd.c_str());
        if (status != 0) {
            Logger::get().err("Iptable support for interception redirect is not set successfully");
        } else {
            first_launch = false;
        }
    }
    **/

    // launch
    pid_t pid;
    if ((pid = fork()) == 0) {
        int status = system(cmd.c_str());
        Logger::get().info("cmd for squid: " + cmd);
        if (status == -1) {
            _exit(EXIT_FAILURE);
        } else {
            _exit(EXIT_SUCCESS);
        }
    } else if (pid == -1) {
        Logger::get().info("fork failure during cache proxy init");
    } else {
        int count = 0;
        int status = 0;
        while ((waitpid(pid, &status, WNOHANG) == 0) && count < 3 && !server_pid) {
            server_pid = check_server();
            usleep(500000);    // minimal init wait time for first launch of the server
            count++;
        }

        Logger::get().info("server_pid is " + std::to_string(server_pid));
        if ( (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) || WIFSTOPPED(status)) {
            Logger::get().err("init failed(system call)");
        } else if (server_pid > 0) {
            Logger::get().info("squid is launched with pid: " + std::to_string(server_pid));
            child_sh_pid = pid;
        } else {
            Logger::get().err("init failed(no server pid returned after several attempts)");
        }
    }
}

/** soft reset with sighup**/
void CacheProxy::reset()
{
    if (server_pid > 0) {
        int status = kill(server_pid, SIGHUP);
        if (status) {
            Logger::get().info("Check status for sending out sighup signal to reload the squid server" + std::to_string(status));
        } else {
            // reconfigure takes less than 1s. should the completeness of the reconfigure. check that by location "reconfigure" in cache.0.log file
            usleep(1000000);
            int pid = check_server();
            if (server_pid == pid) {
                Logger::get().info("Squid has been reset");
            }
        }
    } else {
        Logger::get().info("There is no squid running");
    }
}

/**generate config file to config folder **/
int CacheProxy::generate_config_file(const std::string& config_content, const std::string& file_path, int flag)
{
    // output to the new file
    Logger::get().info("Start to write to configuration file...");
    std::ofstream output_stream(file_path, std::ios_base::out | std::ios_base::trunc);
    std::istringstream iss(config_content);

    for (std::string line; std::getline(iss, line);) {
        // trim
        ltrim(line);
        if (flag == PROXY_CONFIG) {
            std::string key = "pid_filename";
            if (line.rfind(key, 0) == 0) {
                squid_pid_path = line.substr(line.find("/"));
                Logger::get().info("pid file: " + squid_pid_path);
            }
        }
        output_stream << line << '\n';
    }
    if (flag == PROXY_CONFIG) {
        output_stream << "prefer_direct " << prefer_direct << '\n';
        if (squid_pid_path.empty()) {
            Logger::get().err("No pid file configured for the proxy");
        }
    }
    output_stream.close();
    return 0;
}

/** shutdown running proxy process and clean the cache directory, kill all
 * pinger process **/
int CacheProxy::shutnclean()
{
    int count = 0;
    while (server_pid > 0) {
        int status = kill(server_pid, SIGINT);
        Logger::get().info("sigkill is sent to shutnclean the squid server with pid: " + std::to_string(server_pid));
        if (status) {
            // maybe still in shutdown phase, TODO log.err
            Logger::get().info("Check the status for sending SIGINT: " + std::string(strerror(status)));
        } else {
            int ret = waitpid(child_sh_pid, &status, 0);
            if (ret == -1) {
                Logger::get().info("Failed to shutdown squid server with pid: " + std::to_string(server_pid));
                Logger::get().info(strerror(errno));
            } else if (ret == child_sh_pid && WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
                /**
                while (waitpid(child_sh_pid, &status, 0) != child_sh_pid) {
                    usleep(500000);
                }
                **/
                usleep(500000);
                Logger::get().info("Squid is shutdown successfully");
                server_pid = 0; // stop service successfully
            }
        }
        /** pinger no more enabled
        std::string pinger_clean_cmd = "/usr/bin/pkill -f pinger";
        status = system(pinger_clean_cmd.c_str());
        **/
        if (++count > 5) {
            Logger::get().err("Server is not shutdown successfully with serveral attempts");
            return 1;
        }
    }
    return 0;
}

/** By checking pid_filepath, we can get to know the pid of the server if it is running**/
int CacheProxy::check_server()
{
    Logger::get().info("Start checking server");
    int stat_pid = 0;
    if (!fs::exists(squid_pid_path)) {
        Logger::get().info("No such pid file for squid exists" + squid_pid_path);
        return 0;
    }
    FILE *fptr = std::fopen(squid_pid_path.c_str(), "r");
    if (fptr == NULL) {
        Logger::get().info("Fail to open squid pid_file" + squid_pid_path);
    } else {
        char buffer[50];
        if (std::fgets(buffer, 50, fptr) != NULL) {
            stat_pid = std::stoi(std::string(buffer));
        }
    }
    std::fclose(fptr);
    return stat_pid;
}

void get_timestamp(char *buffer, int len)
{
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = std::localtime(&raw_time);
    std::strftime(buffer, len, "%F-%H-%M", time_info);
    return;
}

static inline void ltrim(std::string& str)
{
    // inplace trim
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !isspace(ch);
    }));
}
