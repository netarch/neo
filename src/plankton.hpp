#pragma once

#include <string>

#include "network.hpp"
#include "policy/policy.hpp"
#include "process/choose_conn.hpp"
#include "process/forwarding.hpp"
#include "process/openflow.hpp"
struct State;

class Plankton {
private:
    size_t max_jobs;     // degree of parallelism
    std::string in_file; // input TOML file
    std::string out_dir; // output directory
    Network network;     // network information (inc. dataplane)
    Policies policies;

    /* processes */
    ChooseConnProcess conn_choice;
    ForwardingProcess forwarding;
    OpenflowProcess openflow;

    /* per OS process variables */
    Policy *policy; // the policy being verified

    Plankton();
    void verify_conn();
    void verify_policy(Policy *);
    void check_to_switch_process(State *) const;

public:
    // Disable the copy constructor and the copy assignment operator
    Plankton(const Plankton &) = delete;
    Plankton &operator=(const Plankton &) = delete;

    static Plankton &get();

    void init(bool all_ECs,
              bool rm_out_dir,
              bool dropmon,
              size_t dop,
              int emulations,
              const std::string &input_file,
              const std::string &output_dir);
    int run();

    /***** functions used by the Promela network model *****/
    void initialize(State *);
    void reinit(State *); // used by policies with correlated sub-policies
    void exec_step(State *);
    void report(State *);
    void verify_exit(int);
};
