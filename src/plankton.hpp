#pragma once

#include <string>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/forwarding.hpp"
#include "pan.h"

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
    EqClass   *pre_ec;    // EC of the prerequisite policy
    EqClass   *ec;        // EC to be verified

    /* processes */
    ForwardingProcess   fwd;
    /*
     * NOTE: if there are more than one process, they need to be chosen
     * non-deterministically.
     */

    Plankton();
    void verify(Policy *, EqClass *, EqClass *);
    void dispatch(Policy *, EqClass *, EqClass *);

public:
    // Disable the copy constructor and the copy assignment operator
    Plankton(const Plankton&) = delete;
    Plankton& operator=(const Plankton&) = delete;

    static Plankton& get_instance();

    void init(bool rm_out_dir, size_t dop, bool verbose,
              const std::string& input_file, const std::string& output_dir);
    int run();


    /***** functions called by the Promela network model *****/

    void initialize(State *);
    void execute(State *);
    void report(State *);
};
