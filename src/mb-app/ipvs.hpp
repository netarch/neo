#pragma once

#include <string>

#include "mb-app/mb-app.hpp"

class IPVS : public MB_App {
private:
    std::string config;

private:
    friend class Config;
    IPVS() = default;

public:
    void init() override;  // hard-reset, restart, start
    void reset() override; // soft-reset, reload
};
