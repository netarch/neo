#pragma once

#include <string>
#include <list>

#include "topology.hpp"
#include "policy.hpp"

class Plankton
{
private:
    int max_jobs;
    std::string in_file, out_dir;

    Topology topology;
    std::list<std::shared_ptr<Policy> > policies;

    void load_config();

public:
    Plankton(bool verbose, bool rm_out_dir, int max_jobs,
             const std::string& input_file, const std::string& output_dir);

    void run();
};
