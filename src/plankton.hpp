#pragma once

#include <string>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"
class State;

class Plankton
{
private:
    size_t          max_jobs;   // degree of parallelism
    std::string     in_file;    // input TOML file
    std::string     out_dir;    // output directory
    Network         network;    // network information (inc. dataplane)
    Policies        policies;

    /* per process variables */
    Policy    *policy;    // the policy being verified

    /* processes */
    ForwardingProcess   fwd;
    /*
     * NOTE:
     * If there are more than one process, they need to be chosen
     * non-deterministically.
     */

    Plankton();
    void verify_ec(Policy *);
    void verify_policy(Policy *);

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
