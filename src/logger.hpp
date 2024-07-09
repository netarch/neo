#pragma once

#include <memory>
#include <string>

namespace spdlog {
class logger;
} // namespace spdlog

class Logger {
private:
    std::shared_ptr<spdlog::logger> _stdout_logger;
    std::shared_ptr<spdlog::logger> _file_logger;
    std::string _name;

public:
    Logger(const std::string &name = "");
    Logger(const Logger &)            = delete;
    Logger(Logger &&)                 = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(Logger &&)      = delete;

    void enable_console_logging();
    void disable_console_logging();
    void enable_file_logging(const std::string &filename, bool append = false);
    void disable_file_logging();
    std::string filename();

    void debug(const std::string &);
    void info(const std::string &);
    void warn(const std::string &);
    void error(const std::string &);
    void error(const std::string &, int err_num);
};

extern Logger logger; // global logger
