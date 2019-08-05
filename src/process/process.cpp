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

void Process::exec_step(Network& network __attribute__((unused)),
                        const EqClass& ec __attribute__((unused)))
{
}
