#pragma once

#include "mb-app/mb-app.hpp"
#include <stack>
#include <unordered_map>
#include <vector>
#include <string>

class CacheProxy : public MB_App
{
private:
    int server_pid;
    std::string cmd;
    std::string proxy_config;
    ;

public:
    CacheProxy(const std::shared_ptr<cpptoml::table>&);
    ~CacheProxy() override;

    void init() override;
    void reset() override;
    int shutnclean();
    int generate_config_file(const std::string& config_content);
    int check_server();
};
