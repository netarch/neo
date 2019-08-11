#pragma once

#include "network.hpp"
#include "eqclass.hpp"
#include "pan.h"

enum exec_type {
    INIT = 0,
    INJECT_PACKET = 1,
    PICK_NEIGHBOR = 2,
    FORWARD_PACKET = 3
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
