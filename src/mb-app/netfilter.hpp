#pragma once

#include <string>
#include <cpptoml/cpptoml.hpp>

#include "mb-app/mb-app.hpp"

class NetFilter : public MB_App
{
private:
    int rp_filter;
    std::string rules;
    std::string forwarding_fn, rp_filter_fn;

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
