#pragma once

#include <string>

#include "mb-app/mb-app.hpp"

class Squid : public MB_App {
private:
    pid_t pid;
    std::string config;

    void stop();

private:
    friend class Config;
    Squid() : pid(0) {}

public:
    ~Squid() override;
    void init() override;
    void reset() override;
};
