#include "process/choose_conn.hpp"

#include <cassert>

//#include "network.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

static inline bool is_executable(State *state, int conn)
{
    return state->conn_state[conn].is_executable;
}

bool ChooseConnProcess::has_other_conns(State *state) const
{
    int num_conns = get_num_conns(state);
    int current_conn = get_conn(state);

    for (int conn = 0; conn < num_conns; ++conn) {
        if (conn != current_conn && is_executable(state, conn)) {
            return true;
        }
    }

    return false;
}

void ChooseConnProcess::update_choice_count(State *state) const
{
    int num_conns = get_num_conns(state);
    int active_conns = 0;

    for (int conn = 0; conn < num_conns; ++conn) {
        if (is_executable(state, conn)) {
            active_conns++;
        }
    }

    set_choice_count(state, active_conns);
}

void ChooseConnProcess::exec_step(
    State *state, const Network& network __attribute__((unused)))
{
    if (!enabled) {
        return;
    }

    // switch to the chosen connection
    int choice = get_choice(state);
    int num_conns = get_num_conns(state);

    for (int conn = 0; conn < num_conns; ++conn) {
        if (is_executable(state, conn)) {
            if (choice-- == 0) {
                set_conn(state, conn);
                break;
            }
        }
    }
    assert(choice == -1);   // a connection must be chosen
    set_choice_count(state, 1); // back to forwarding
}
