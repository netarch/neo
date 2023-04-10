#include "logger.hpp"

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <boost/stacktrace.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// [second.microsecond] [pid] [log_level] log_msg
#define LOG_PATTERN "[%E.%f] [%P] [%^%L%L%$] %v"

using namespace std;
namespace st = boost::stacktrace;

Logger logger; // global logger

Logger::Logger(const string &name) : _name(name) {}

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

void Logger::enable_file_logging(const string &filename, bool append) {
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

string Logger::filename() {
    if (this->_file_logger) {
        return static_pointer_cast<spdlog::sinks::basic_file_sink_st>(
                   this->_file_logger->sinks()[0])
            ->filename();
    }
    return "";
}

void Logger::debug(const string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->debug(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->debug(msg);
    }
}

void Logger::info(const string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->info(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->info(msg);
    }
}

void Logger::warn(const string &msg) {
    if (this->_stdout_logger) {
        this->_stdout_logger->warn(msg);
    }
    if (this->_file_logger) {
        this->_file_logger->warn(msg);
    }
}

void Logger::error(const string &msg) {
    auto trace = st::stacktrace();
    string msg_with_trace = msg;

    if (!trace.empty()) {
        msg_with_trace += "\n" + st::to_string(trace);
    }

    if (this->_stdout_logger) {
        this->_stdout_logger->error(msg_with_trace);
    }
    if (this->_file_logger) {
        this->_file_logger->error(msg_with_trace);
    }

    throw runtime_error(msg);
}

void Logger::error(const string &msg, int err_num) {
    locale_t locale = newlocale(LC_ALL_MASK, "", 0);
    string err_str = strerror_l(err_num, locale);
    freelocale(locale);
    error(msg + ": " + err_str);
}
