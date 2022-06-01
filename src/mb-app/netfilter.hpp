#pragma once

#include <string>

#include "mb-app/mb-app.hpp"

class NetFilter : public MB_App {
private:
    int rp_filter;
    std::string rules;

private:
    friend class Config;
    NetFilter() = default;

public:
    void init() override;  // hard-reset, restart, start
    void reset() override; // soft-reset, reload
};
