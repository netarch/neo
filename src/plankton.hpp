#pragma once

#include <string>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"
#include "process/openflow.hpp"
class State;

class Plankton
{
private:
    size_t          max_jobs;   // degree of parallelism
    std::string     in_file;    // input TOML file
    std::string     out_dir;    // output directory
    Network         network;    // network information (inc. dataplane)
    Policies        policies;
    EqClasses       all_ECs, owned_ECs; // policy-oblivious ECs

    /* Plankton processes (all processes are enabled by default) */
    ForwardingProcess   forwarding;
    OpenflowProcess     openflow;

    /* per OS process variables */
    Policy  *policy;            // the policy being verified

    Plankton();
    void compute_policy_oblivious_ecs();
    void verify_ec(Policy *);
    void verify_policy(Policy *);
    void process_switch(State *) const;

public:
    // Disable the copy constructor and the copy assignment operator
    Plankton(const Plankton&) = delete;
    Plankton& operator=(const Plankton&) = delete;

    static Plankton& get();

    void init(bool all_ECs, bool rm_out_dir, size_t dop, bool latency,
              bool verbose, const std::string& input_file,
              const std::string& output_dir);
    int run();

    /***** functions used by the Promela network model *****/
    void initialize(State *);
    void exec_step(State *);
    void report(State *);
    void verify_exit(int);
};
