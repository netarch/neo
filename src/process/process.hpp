#pragma once

#include <string>

class Network;
struct State;

enum pid {
    forwarding,
    openflow
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

    virtual void exec_step(State *, Network&) = 0;
};
