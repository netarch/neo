#include "logger.hpp"

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

// [second.microsecond] [pid] [log_level] log_msg
#define LOG_PATTERN "[%E.%f] [%P] [%^%L%L%$] %v"

Logger logger; // global logger

Logger::Logger(const std::string &name) : _name(name) {}

void Logger::enable_console_logging() {
    if (!this->_stdout_logger) {
        this->_stdout_logger = spdlog::stdout_color_st(this->_name + ":stdout");
        this->_stdout_logger->set_pattern(LOG_PATTERN);
#ifdef ENABLE_DEBUG
        this->_stdout_logger->set_level(spdlog::level::debug);
#else
        this->_stdout_logger->set_level(spdlog::level::info);
#endif
        this->_stdout_logger->flush_on(spdlog::level::debug);
    }
}

void Logger::disable_console_logging() {
    if (this->_stdout_logger) {
        this->_stdout_logger = nullptr;
        spdlog::drop(this->_name + ":stdout");
    }
}

void Logger::enable_file_logging(const std::string &filename, bool append) {
    if (this->filename() != filename) {
        disable_file_logging();
        this->_file_logger =
            spdlog::basic_logger_st(this->_name + ":file", filename,
                                    /* truncate */ !append);
        this->_file_logger->set_pattern(LOG_PATTERN);
#ifdef ENABLE_DEBUG
        this->_file_logger->set_level(spdlog::level::debug);
#else
        this->_file_logger->set_level(spdlog::level::info);
#endif
        this->_file_logger->flush_on(spdlog::level::debug);
    }
}

void Logger::disable_file_logging() {
    if (this->_file_logger) {
        this->_file_logger = nullptr;
        spdlog::drop(this->_name + ":file");
    }
}

std::string Logger::filename() {
    if (this->_file_logger) {
        return std::static_pointer_cast<spdlog::sinks::basic_file_sink_st>(
                   this->_file_logger->sinks()[0])
            ->filename();
    }
    return "";
}

void Logger::debug(const std::string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->debug(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->debug(msg);
    }
}

void Logger::info(const std::string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->info(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->info(msg);
    }
}

void Logger::warn(const std::string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->warn(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->warn(msg);
    }
}

void Logger::error(const std::string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->error(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->error(msg);
    }
    exit(1);
}

void Logger::error(const std::string &msg, int err_num) {
    locale_t locale = newlocale(LC_ALL_MASK, "", 0);
    std::string err_str = strerror_l(err_num, locale);
    freelocale(locale);
    error(msg + ": " + err_str);
}
