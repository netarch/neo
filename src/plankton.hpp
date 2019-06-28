#pragma once

#include <string>
#include <unordered_set>
#include <list>
#include <memory>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"

class Plankton
{
private:
    size_t              max_jobs;   // degree of parallelism
    std::string         in_file;    // input TOML file
    std::string         out_dir;    // output directory
    Network             network;    // network information (inc. dataplane)
    std::list<std::shared_ptr<Policy> > policies;

    ForwardingProcess   fwd;

    int verify(const std::shared_ptr<EqClass>&, const std::shared_ptr<Policy>&);

public:
    Plankton(bool verbose, bool rm_out_dir, size_t max_jobs,
             const std::string& input_file, const std::string& output_dir);

    void run();

    /******* functions called by the Promela network model *******/
};
