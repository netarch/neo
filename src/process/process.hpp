#pragma once

enum pid {
    choose_conn,
    forwarding,
    openflow
};

class Process {
protected:
    bool _enabled;

public:
    Process() : _enabled(false) {}
    virtual ~Process() = default;

    void enable() { _enabled = true; }
    void disable() { _enabled = false; }
    bool enabled() const { return _enabled; }

    virtual void exec_step() = 0;
    virtual void reset() {}
};
