#include "lib/logger.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <clocale>
#include <stdexcept>

Logger::Logger(): verbose(false)
{
}

void Logger::log(const std::string& type, const std::string& msg)
{
    // write log if the file is open
    if (logfile.is_open()) {
        logfile << "[" + type + "] " + msg << std::endl;
    }
}

Logger::~Logger()
{
    if (logfile.is_open()) {
        logfile.close();
    }
}

Logger& Logger::get()
{
    static Logger instance;
    return instance;
}

void Logger::reopen()
{
    if (logfile.is_open()) {
        logfile.close();
    }
    if (!filename.empty()) {
        logfile.open(filename, std::ios_base::app);
        if (logfile.fail()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
}

void Logger::set_file(const std::string& fn)
{
    if (logfile.is_open()) {
        logfile.close();
    }
    filename = fn;
    if (!filename.empty()) {
        logfile.open(filename, std::ios_base::app);
        if (logfile.fail()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
}

void Logger::set_verbose(bool v)
{
    verbose = v;
}

std::string Logger::get_file() const
{
    return filename;
}

void Logger::print(const std::string& msg)
{
    if (logfile.is_open()) {
        logfile << msg << std::endl;
    }
}

void Logger::out(const std::string& msg)
{
    std::cout << msg << std::endl;
}

#ifdef DEBUG
void Logger::debug(const std::string& msg)
{
    log("DEBUG", msg);
    if (verbose) {
        std::cout << "[DEBUG] " << msg << std::endl;
    }
}
#else
void Logger::debug(const std::string& msg __attribute__((unused)))
{
}
#endif

void Logger::info(const std::string& msg)
{
    log("INFO", msg);
    if (verbose) {
        std::cout << "[INFO] " << msg << std::endl;
    }
}

void Logger::warn(const std::string& msg)
{
    log("WARN", msg);
    if (verbose) {
        std::cout << "[WARN] " << msg << std::endl;
    } else {
        std::cerr << "[WARN] " << msg << std::endl;
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
