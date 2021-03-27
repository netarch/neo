#pragma once

#include "process/process.hpp"
class Network;
struct State;

/*
 * Choose the next communication non-deterministically
 */
class ChooseCommProcess : public Process
{
public:
    ChooseCommProcess() = default;

    bool has_other_comms(State *) const;
    void update_choice_count(State *) const;

    void exec_step(State *, Network&) override;
};
