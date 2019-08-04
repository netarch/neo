#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void initialize(struct State *state)
{
    plankton.initialize();
    void *p;
    memcpy(&p, state->state, sizeof(void *));
    Logger::get_instance().info("state: " + std::to_string((uint64_t)p));
}
