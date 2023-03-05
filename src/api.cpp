#include "api.hpp"

#include "model-access.hpp"
#include "plankton.hpp"

class API {
public:
    static void initialize(State *state) {
        Model::get().set_state(state);
        Plankton::get().initialize();
    }

    static void exec_step(State *state) {
        Model::get().set_state(state);
        Plankton::get().exec_step();
    }

    static void report(State *state) {
        Model::get().set_state(state);
        Plankton::get().report();
    }

    static void verify_exit(int status) { Plankton::get().verify_exit(status); }
};

void initialize(State *state) {
    API::initialize(state);
}

void exec_step(State *state) {
    API::exec_step(state);
}

void report(State *state) {
    API::report(state);
}

void verify_exit(int status) {
    API::verify_exit(status);
}
