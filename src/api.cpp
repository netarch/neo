#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void initialize(State *state)
{
    plankton.initialize(state);
}

void exec_step(State *state)
{
    plankton.exec_step(state);
}

void report(State *state)
{
    plankton.report(state);
}
