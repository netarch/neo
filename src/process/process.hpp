#pragma once

#include "network.hpp"
#include "eqclass.hpp"
#include "pan.h"

enum exec_type {
    INJECT_PACKET = 0,
    FOWARD_PACKET = 1
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
    virtual void exec_step(State *) const = 0;
};
