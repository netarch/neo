#include "process/choose_comm.hpp"

#include <cassert>

//#include "network.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "model.h"

static bool is_finished(State *state, int comm)
{
    return ((state->comm_state[comm].fwd_mode == fwd_mode::ACCEPTED &&
             PS_IS_LAST(state->comm_state[comm].proto_state)) ||
            state->comm_state[comm].fwd_mode == fwd_mode::DROPPED);
}

bool ChooseCommProcess::has_other_comms(State *state) const
{
    int num_comms = get_num_comms(state);
    int current_comm = get_comm(state);

    for (int comm = 0; comm < num_comms; ++comm) {
        if (comm != current_comm && !is_finished(state, comm)) {
            return true;
        }
    }

    return false;
}

void ChooseCommProcess::update_choice_count(State *state) const
{
    int num_comms = get_num_comms(state);
    int active_comms = 0;

    for (int comm = 0; comm < num_comms; ++comm) {
        if (!is_finished(state, comm)) {
            active_comms++;
        }
    }

    set_choice_count(state, active_comms);
}

void ChooseCommProcess::exec_step(State *state, Network& network __attribute__((unused)))
{
    if (!enabled) {
        return;
    }

    // switch to the chosen communication
    int choice = get_choice(state);
    int num_comms = get_num_comms(state);

    for (int comm = 0; comm < num_comms; ++comm) {
        if (!is_finished(state, comm)) {
            if (choice-- == 0) {
                set_comm(state, comm);
                break;
            }
        }
    }
    assert(choice == -1);   // a communication must be chosen
    set_choice_count(state, 1); // back to forwarding
}
