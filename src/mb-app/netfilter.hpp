#pragma once

#include "mb-app/mb-app.hpp"

class NetFilter : public MB_App
{
private:
    ;

public:
    /*
     * Don't start emulation processes in the constructor.
     * Only read the configurations in constructors and later start the
     * emulation in init().
     */
    NetFilter(const std::shared_ptr<cpptoml::table>&);
    ~NetFilter() override;

    void init() override;   // hard-reset, restart, start
    void reset() override;  // soft-reset, reload
};
