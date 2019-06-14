#pragma once

#include <string>
#include <list>

#include "network.hpp"
#include "eqclass.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"

class Plankton
{
private:
    int                 max_jobs;   // degree of parallelism
    std::string         in_file;    // input TOML file
    std::string         out_dir;    // output directory
    Network             network;    // network information (inc. dataplane)
    std::set<EqClass>   ECs;        // ECs to be verified
    std::list<std::shared_ptr<Policy> > policies;

    ForwardingProcess   fwd;

    void compute_ec();

public:
    Plankton(bool verbose, bool rm_out_dir, int max_jobs,
             const std::string& input_file, const std::string& output_dir);

    void run();

    /******* functions called by the Promela network model *******/
};
