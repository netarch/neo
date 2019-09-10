#pragma once

#include "mb-app/mb-app.hpp"

class MB_Env
{
public:
    virtual ~MB_Env() = default;

    virtual void run(void (*)(MB_App *), MB_App *) = 0;
};
