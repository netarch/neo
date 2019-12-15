#include <sys/wait.h>
#include <sys/types.h> // for pid_t 
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <iostream>
#include <ios>
#include <cstdio>
#include <fcntl.h>

#include <fstream>
#include "mb-app/cache-proxy.hpp"
#include "lib/fs.hpp"
#include "lib/logger.hpp"

const std::string curr_dir = fs::get_cwd();
std::string config_path(curr_dir + "/" + "examples/configs/squid/");

const std::string proxy_path = "/usr/sbin/squid";
const std::string squid_pid = "/var/run/squid.pid";
const std::string log_dir = curr_dir + "/" + "examples/configs/squid/log/";
const std::string cache_log = curr_dir + "/" + "examples/configs/squid/log/cache.log";        // debugging
const std::string access_log = curr_dir + "/" + "examples/configs/squid/log/access.log";       // client
const std::string cache_store_log = curr_dir + "/" + "examples/configs/squid/log/store.log";  // stored cache object if on disk

void get_timestamp(char *buffer, int len);
CacheProxy::CacheProxy(const std::shared_ptr<cpptoml::table>& node)
{
    server_pid = 0;
    std::string squid("squid");
    char buff[80];
    if (node) {
        auto node_config = node->get_as<std::string>("config");
        auto app = node->get_as<std::string>("app");
        if (app && squid.compare(*app)) {
            auto config = node->get_as<std::string>("config");
            proxy_config = *config;
            get_timestamp(&buff[0], 80);
            config_path = config_path + "config-" + std::string(buff);
            if (generate_config_file(proxy_config) == 0) {
                Logger::get().info("config file generated");
            } else {
                exit(EXIT_FAILURE);
            }
        } else {
            Logger::get().info("it is not a squid node");
        }
    } else {
        Logger::get().info("node_config is empty, please check ");
        exit(EXIT_FAILURE); // terminate the calling process
    }
    cmd = proxy_path + " -NYC" + " -f " + config_path;
    Logger::get().info("Cache Proxy is created..");

    // set the log file
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
}

CacheProxy::~CacheProxy()
{
    shutnclean();
    Logger::get().info("Cache Proxy is destroyed..");
}

/** clean cache storage and launch server in child process**/
void CacheProxy::init()
{
    if (server_pid > 0) {
        int status = shutnclean();
        if (status) {
            Logger::get().err("init failure because server is not shutdown successfully");
        };
    }
    // launch
    pid_t pid;
    int status = -2;
    if ((pid = fork()) == 0) {
        status = system(cmd.c_str());
        Logger::get().info("cmd for launching squid: " + cmd);
        Logger::get().info("init status for squid:" + std::to_string(status));
        _exit(EXIT_SUCCESS);
    } else if (pid == -1) {
        Logger::get().info("fork failure during cache proxy init");
    } else {
        int count = 0;
        while (status == -2 && !server_pid && count < 5) {
            server_pid = check_server();
            usleep(500000);    // minimal init wait time
            std::cout << "check server with pid: " << std::to_string(server_pid) << std::endl;
            count++;
        }
        if (server_pid) {
            Logger::get().info("squid is launched");
        } else {
            Logger::get().err("init failed");
        }
    }
}

/** soft reset with sighup**/
void CacheProxy::reset()
{
    if (server_pid > 0) {
        int status = kill(server_pid, SIGHUP);
        if (status) {
            Logger::get().info("There has been.infoor in sending out sighup signal to reload the squid server");
        } else {
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
int CacheProxy::generate_config_file(const std::string& config_content)
{
    // output to the new file
    Logger::get().info("Start to write to configuration file...");
    std::ofstream output_stream(config_path, std::ios_base::out | std::ios_base::trunc);
    output_stream << config_content;
    output_stream.close();
    return 0;
}

/** shutdown running proxy process and clean the cache directory, kill all
 * pinger process **/
int CacheProxy::shutnclean()
{
    int count = 0;
    while (server_pid > 0) {
        Logger::get().info(std::to_string(server_pid));
        int status = kill(server_pid, SIGINT);
        Logger::get().info("shutdown with kill to server: " + std::to_string(server_pid));
        if (status) {
            Logger::get().info("There is.infoor in sending SIGINT");
        } else {
            server_pid = 0; // stop service successfully
        }
        std::string pinger_clean_cmd = "/usr/bin/pkill -f pinger";
        status = system(pinger_clean_cmd.c_str());
        if (count > 5) {
            return 1;
        }
    }
    return 0;
}

/** By checking pid_filepath, we can get to know the pid of the server if it is running**/
int CacheProxy::check_server()
{
    int stat_pid = 0;
    FILE *fptr = fopen(squid_pid.c_str(), "r");
    if (fptr == NULL) {
        Logger::get().info(squid_pid);
    } else {
        char buffer[50];
        if (fgets(buffer, 50, fptr) != NULL) {
            stat_pid = std::stoi(std::string(buffer));
        }
    }
    return stat_pid;
}

void get_timestamp(char *buffer, int len)
{
    time_t raw_time;
    struct tm *time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(buffer, len, "%F-%H-%M", time_info);
}
