#ifndef PLANKTON_HPP
#define PLANKTON_HPP

#include <string>

#include "topology.hpp"
#include "lib/logger.hpp"

class Plankton
{
private:
    Logger& logger;
    int max_jobs;
    std::string in_file, out_dir;

    Topology topology;
    //policies;

    void load_config();

public:
    Plankton(bool, int, const std::string&, const std::string&);

    void run();
};

#endif
