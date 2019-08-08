#pragma once

#include "network.hpp"
#include "eqclass.hpp"

#include "pan.h"

enum step_type {
    INIT = 0,
    INJECT_PACKET = 1,
    FOWARD_PACKET = 2
};

class Process
{
protected:
    bool enabled;

public:
    Process();

    void enable();
    void disable();
    bool is_enabled() const;
    virtual void exec_step(State *state) const = 0;
};
