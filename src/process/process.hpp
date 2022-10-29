#pragma once

class Network;

enum pid {
    choose_conn,
    forwarding,
    openflow
};

class Process {
protected:
    bool enabled;

public:
    Process();

    void enable();
    void disable();
    bool is_enabled() const;

    virtual void exec_step(const Network &) = 0;
};
