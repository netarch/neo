#ifndef PLANKTON_HPP
#define PLANKTON_HPP

#include <string>

#include "lib/logger.hpp"

class Plankton
{
private:
    int max_jobs;
    Logger& logger;
    std::string in_dir, out_dir;

public:
    Plankton(int, const std::string&, const std::string&);

    void run();
};

#endif
