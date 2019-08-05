#pragma once

#include "network.hpp"
#include "eqclass.hpp"

class Process
{
protected:
    bool enabled;

public:
    Process();

    void enable();
    void disable();
    bool is_enabled() const;

    virtual void exec_step(Network&, const EqClass&);
};
