#pragma once

class Network;
struct State;

enum pid {
    choose_comm,
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
