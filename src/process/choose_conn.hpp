#pragma once

#include "process/process.hpp"

/**
 * Choose the next connection non-deterministically
 */
class ChooseConnProcess : public Process {
public:
    void update_choice_count() const;
    void exec_step() override;
};
