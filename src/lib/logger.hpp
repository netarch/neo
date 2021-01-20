#pragma once

#include <memory>
#include <string>

namespace spdlog
{
class logger;
} // namespace spdlog

class Logger
{
private:
    static std::shared_ptr<spdlog::logger> stdout_logger;
    static std::shared_ptr<spdlog::logger> file_logger;

public:
    static void enable_console_logging();
    static void disable_console_logging();
    static void enable_file_logging(const std::string& filename);
    static void disable_file_logging();
    static std::string filename();

    static void debug(const std::string&);
    static void info(const std::string&);
    static void warn(const std::string&);
    static void error(const std::string&);
    static void error(const std::string&, int err_num);
};
