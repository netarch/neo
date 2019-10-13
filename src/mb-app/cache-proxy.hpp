#pragma once

#include "mb-app/mb-app.hpp"
#include <stack>
#include <unordered_map>
#include <vector>

class CacheProxy : public MB_App
{
private:
    int server_pid;
    char *curr_dir;
    char *config_path;
    char const *proxy_path;
    std::vector<char*> proxy_argv;
    std::string proxy_config;
    // internal state of the server
    std::unordered_map<std::string, std::string> host_access_count;
    std::stack<std::string> host_access_history;
    ;

public:
    CacheProxy(const std::shared_ptr<cpptoml::table>&);
    ~CacheProxy() override;

    void init() override;
    void reset() override;
    int shutnclean();
    void parse_node();
};
