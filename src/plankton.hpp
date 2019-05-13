#ifndef PLANKTON_HPP
#define PLANKTON_HPP

#include <string>

#include "lib/logger.hpp"

class Plankton
{
private:
    int max_jobs;
    Logger& logger;
    std::string in_file, out_dir;

    //void input();

public:
    Plankton(int, const std::string&, const std::string&);

    void run();
};

#endif
