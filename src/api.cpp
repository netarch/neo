#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void initialize(struct State *state)
{
    plankton.initialize();
    Logger::get_instance().info("Initialization done");
    assert(state);
}

void execute(struct State *state)
{
    plankton.get_forwarding_process().exec_step(state);
}
