#include "lib/logger.hpp"

#include <cstring>
#include <clocale>
#include <string>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

// [second.microsecond] [pid] [log_level] log_msg
#define LOG_PATTERN "[%E.%f] [%P] [%^%L%L%$] %v"

std::shared_ptr<spdlog::logger> Logger::stdout_logger = nullptr;
std::shared_ptr<spdlog::logger> Logger::file_logger = nullptr;

void Logger::enable_console_logging()
{
    if (!stdout_logger) {
        stdout_logger = spdlog::stdout_color_st("stdout_logger");
        stdout_logger->set_pattern(LOG_PATTERN);
#ifdef ENABLE_DEBUG
        stdout_logger->set_level(spdlog::level::debug);
#else
        stdout_logger->set_level(spdlog::level::info);
#endif
        stdout_logger->flush_on(spdlog::level::debug);
    }
}

void Logger::disable_console_logging()
{
    if (stdout_logger) {
        stdout_logger = nullptr;
        spdlog::drop("stdout_logger");
    }
}

void Logger::enable_file_logging(const std::string& filename)
{
    if (Logger::filename() != filename) {
        disable_file_logging();
        file_logger = spdlog::basic_logger_st("file_logger", filename, /* truncate */false);
        file_logger->set_pattern(LOG_PATTERN);
#ifdef ENABLE_DEBUG
        file_logger->set_level(spdlog::level::debug);
#else
        file_logger->set_level(spdlog::level::info);
#endif
        file_logger->flush_on(spdlog::level::debug);
    }
}

void Logger::disable_file_logging()
{
    if (file_logger) {
        file_logger = nullptr;
        spdlog::drop("file_logger");
    }
}

std::string Logger::filename()
{
    if (file_logger) {
        return std::static_pointer_cast<spdlog::sinks::basic_file_sink_st>(
                   file_logger->sinks()[0])->filename();
    }
    return "";
}

void Logger::debug(const std::string& msg)
{
    if (stdout_logger) {
        stdout_logger->debug(msg);
    }
    if (file_logger) {
        file_logger->debug(msg);
    }
}

void Logger::info(const std::string& msg)
{
    if (stdout_logger) {
        stdout_logger->info(msg);
    }
    if (file_logger) {
        file_logger->info(msg);
    }
}

void Logger::warn(const std::string& msg)
{
    if (stdout_logger) {
        stdout_logger->warn(msg);
    }
    if (file_logger) {
        file_logger->warn(msg);
    }
}

void Logger::error(const std::string& msg)
{
    if (stdout_logger) {
        stdout_logger->error(msg);
    }
    if (file_logger) {
        file_logger->error(msg);
    }
    throw std::runtime_error(msg);
}

void Logger::error(const std::string& msg, int err_num)
{
    locale_t locale = newlocale(LC_ALL_MASK, "", 0);
    std::string err_str = strerror_l(err_num, locale);
    freelocale(locale);
    error(msg + ": " + err_str);
}
