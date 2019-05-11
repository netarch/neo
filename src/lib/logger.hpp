#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>

class Logger
{
private:
    Logger() {}
    void log(const std::string&, const std::string&);

    std::string filename;
    std::ofstream logfile;

public:
    // Disable the copy constructor and assignment operator
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    static Logger& get_instance();

    void set_file(const std::string&);

    void out(const std::string&);   // print to stdout
    void info(const std::string&);  // log info
    void warn(const std::string&);  // log warning
    void err(const std::string&);   // log error and print to stderr
};

#endif
