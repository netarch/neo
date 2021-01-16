#include "process/process.hpp"

#include "lib/logger.hpp"

Process::Process(): enabled(true)
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

std::string process_id_to_str(int process_id)
{
    switch (process_id) {
        case pid::FORWARDING:
            return "pid::FORWARDING";
        case pid::OPENFLOW:
            return "pid::OPENFLOW";
        default:
            Logger::get().err("Unknown process id " + std::to_string(process_id));
            return "";
    }
}
