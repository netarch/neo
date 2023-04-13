#pragma once

#include <string>

#include "logger.hpp"

enum pid {
    choose_conn,
    forwarding,
    openflow
};

static inline std::string pid_str(int n) {
    switch (n) {
    case pid::choose_conn: {
        return "choose_conn";
    }
    case pid::forwarding: {
        return "forwarding";
    }
    case pid::openflow: {
        return "openflow";
    }
    default: {
        logger.error("Invalid process id " + std::to_string(n));
    }
    }

    return "";
}

class Process {
public:
    virtual ~Process() = default;

    virtual void exec_step() = 0;
    virtual void reset() {}
};
