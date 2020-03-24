#pragma once

#include <string>
#include <cpptoml/cpptoml.hpp>

#include "mb-app/mb-app.hpp"

class Squid : public MB_App
{
private:
    std::string config;
    pid_t pid;

    void stop();

public:
    /*
     * Don't start emulation processes in the constructor.
     * Only read the configurations in constructors and later start the
     * emulation in init().
     */
    Squid(const std::shared_ptr<cpptoml::table>&);
    ~Squid() override;

    void init() override;
    void reset() override;
};
