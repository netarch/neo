#pragma once

#include "mb-app/mb-app.hpp"
#include <string>

class CacheProxy : public MB_App
{
private:
    int server_pid;
    int first_launch;
    std::string cmd;
    std::string squid_pid_path;
    std::string curr_dir;
    std::string proxy_path;
    // std::string iptable_cmd;
    ;

public:
    CacheProxy(const std::shared_ptr<cpptoml::table>&);
    ~CacheProxy() override;

    int child_sh_pid;
    std::string prefer_direct;
    void init() override;
    void reset() override;
    int shutnclean();
    int generate_config_file(const std::string& config_content, const std::string& file_path, int flag);
    int check_server();
};
