#include "process/process.hpp"

Process::Process(): enabled(false)
{
}

void Process::enable()
{
    enabled = true;
}

void Process::disable()
{
    enabled = false;
}

bool Process::is_enabled() const
{
    return enabled;
}
