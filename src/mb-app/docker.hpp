#pragma once

#include "interface.hpp"
#include "mb-app/mb-app.hpp"
#include <map>
#include <vector>

class Docker : public MB_App {
private:
    friend class Config;

    // docker run --name <container_name> <image> <command>
    std::string container_name;
    std::string image;
    std::vector<std::string> cmd;

    // ???
    const std::string config;

    // pid of container
    pid_t pid;

public:
    // must be initialized with a list of interfaces
    Docker();

    ~Docker() override;

    void init() override;  // hard-reset, restart, start
    void reset() override; // soft-reset, reload
};