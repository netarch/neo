#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void initialize(struct State *state)
{
    plankton.initialize(state);
}

void execute(struct State *state)
{
    plankton.execute(state);
}
