#pragma once

#include <string>

#include "network.hpp"
struct State;

enum pid {
    FORWARDING,
    OPENFLOW
};

std::string process_id_to_str(int);

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
