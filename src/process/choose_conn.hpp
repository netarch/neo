#pragma once

#include "process/process.hpp"
class Network;
struct State;

/*
 * Choose the next connection non-deterministically
 */
class ChooseConnProcess : public Process
{
public:
    ChooseConnProcess() = default;

    bool has_other_conns(State *) const;
    void update_choice_count(State *) const;

    void exec_step(State *, const Network&) override;
};
