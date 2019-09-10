#pragma once

#include "mb-app/mb-app.hpp"

class NetFilter : public MB_App
{
private:
    ;

public:
    NetFilter(const std::shared_ptr<cpptoml::table>&);
    ~NetFilter() override;

    void init() override;   // hard-reset, restart
    void reset() override;  // soft-reset, reload
};
