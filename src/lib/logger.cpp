#include <iostream>
#include <string>
#include <cstring>
#include <clocale>
#include <stdexcept>

#include "lib/logger.hpp"

void Logger::log(const std::string& type, const std::string& msg)
{
    // open file if needed
    if (!logfile.is_open() && !filename.empty()) {
        logfile.open(filename);
        if (logfile.fail()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }

    // write log if the file is open
    if (logfile.is_open()) {
        logfile << "-- [" + type + "] " + msg << std::endl;
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

void Logger::set_verbose(bool v)
{
    verbose = v;
}

void Logger::out(const std::string& msg)
{
    std::cout << msg << std::endl;
}

void Logger::info(const std::string& msg)
{
    log("INFO", msg);
    if (verbose) {
        std::cout << "Info: " << msg << std::endl;
    }
}

void Logger::warn(const std::string& msg)
{
    log("WARN", msg);
    if (verbose) {
        std::cout << "Warning: " << msg << std::endl;
    }
}

void Logger::err(const std::string& msg)
{
    log("ERRO", msg);
    throw std::runtime_error(msg);
}

void Logger::err(const std::string& msg, int errnum)
{
    locale_t locale = newlocale(LC_ALL_MASK, "", 0);
    std::string err_str = strerror_l(errnum, locale);
    freelocale(locale);
    err(msg + ": " + err_str);
}
