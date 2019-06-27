#pragma once

#include <string>
#include <unordered_set>
#include <list>
#include <memory>

#include "network.hpp"
#include "eqclasses.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"

class Plankton
{
private:
    size_t              max_jobs;   // degree of parallelism
    std::string         in_file;    // input TOML file
    std::string         out_dir;    // output directory
    Network             network;    // network information (inc. dataplane)
    EqClasses           ECs;        // ECs to be verified
    std::list<std::shared_ptr<Policy> > policies;

    ForwardingProcess   fwd;

    void compute_ecs();
    int verify_ec(const std::shared_ptr<EqClass>&);

public:
    Plankton(bool verbose, bool rm_out_dir, size_t max_jobs,
             const std::string& input_file, const std::string& output_dir);

    void run();

    /******* functions called by the Promela network model *******/
};
