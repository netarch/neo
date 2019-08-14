#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void initialize(State *state)
{
    plankton.initialize(state);
}

void execute(State *state)
{
    plankton.execute(state);
}

void report(State *state)
{
    plankton.report(state);
}
