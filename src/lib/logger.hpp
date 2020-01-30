#pragma once

#include <fstream>
#include <string>

class Logger
{
private:
    std::string filename;
    std::ofstream logfile;
    bool verbose;

    Logger();
    void log(const std::string&, const std::string&);

public:
    // Disable the copy constructor and the copy assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger();

    static Logger& get();

    void reopen();
    void set_file(const std::string&);
    void set_verbose(bool);

    void print(const std::string&);     // print directly to log file
    void out(const std::string&);       // print to stdout
    void debug(const std::string&);     // log debugging messages
    void info(const std::string&);      // log info
    void warn(const std::string&);      // log warning
    void err(const std::string&);       // log error and throw exception
    void err(const std::string&, int);  // log error and throw exception
};
