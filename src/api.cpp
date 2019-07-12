#include <cstring>

#include "api.hpp"
#include "plankton.hpp"
#include "pan.h"

static Plankton& plankton = Plankton::get_instance();

void update_fib(struct State *state)
{
    plankton.update_fib();
    void *p;
    memcpy(&p, state->state, sizeof(void *));
    Logger::get_instance().info("state: " + std::to_string((uint64_t)p));
}
