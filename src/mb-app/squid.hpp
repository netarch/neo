#pragma once

#include <string>

#include "mb-app/mb-app.hpp"

class Squid : public MB_App
{
private:
    std::string config;
    pid_t pid;

    void stop();

private:
    friend class Config;
    Squid(): pid(0) {}

public:
    ~Squid() override;
    void init() override;
    void reset() override;
};
