#include <iostream>
#include <string>

#include "lib/logger.hpp"

void Logger::log(const std::string& type, const std::string& msg)
{
    // open file if needed
    if (!logfile.is_open() && !filename.empty()) {
        logfile.open(filename);
        if (logfile.fail()) {
            err("Failed to open log file: " + filename);
        }
    }

    // write log if the file is open
    if (logfile.is_open()) {
        logfile << "==[" + type + "]== " + msg << std::endl;
    }
}

Logger& Logger::get_instance()
{
    static Logger instance;
    return instance;
}

void Logger::set_file(const std::string& fn)
{
    filename = fn;
}

void Logger::out(const std::string& msg)
{
    std::cout << msg << std::endl;
}

void Logger::info(const std::string& msg)
{
    log("INFO", msg);
}

void Logger::warn(const std::string& msg)
{
    log("WARN", msg);
}

void Logger::err(const std::string& msg)
{
    log("ERRO", msg);
    std::cerr << "Error: " << msg << std::endl;
}
