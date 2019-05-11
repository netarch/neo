#ifndef PLANKTON_HPP
#define PLANKTON_HPP

#include <string>

#include "lib/logger.hpp"

class Plankton
{
private:
    std::string in_dir, out_dir;
    Logger& logger;

public:
    Plankton(const std::string&, const std::string&);

    void run();
};

#endif
