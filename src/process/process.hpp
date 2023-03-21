#pragma once

enum pid {
    choose_conn,
    forwarding,
    openflow
};

class Process {
public:
    virtual ~Process() = default;

    virtual void exec_step() = 0;
    virtual void reset() {}
};
