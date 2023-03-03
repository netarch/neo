#include "process/process.hpp"

#include "logger.hpp"

Process::Process() : enabled(true) {}

void Process::enable() {
    enabled = true;
}

void Process::disable() {
    enabled = false;
}

bool Process::is_enabled() const {
    return enabled;
}
