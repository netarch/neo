#include "policy/consistency.hpp"

#include "process/forwarding.hpp"
#include "model.h"

std::string ConsistencyPolicy::to_string() const
{
    std::string ret = "consistency of ";
    for (Policy *p : correlated_policies) {
        ret += p->to_string() + ", ";
    }
    ret.pop_back();
    ret.pop_back();
    return ret;
}

void ConsistencyPolicy::init(State *state)
{
    first_run = true;
    correlated_policies[state->comm]->init(state);
}

int ConsistencyPolicy::check_violation(State *state)
{
    correlated_policies[state->comm]->check_violation(state);

    if (state->choice_count == 0) {
        // for the first execution, store the verification result
        if (first_run) {
            result = state->violated;
            first_run = false;
        }

        // check for consistency
        if (state->violated != result) {
            state->violated = true;
            state->choice_count = 0;
            return POL_NULL;
        }

        // next subpolicy
        if ((size_t)state->comm + 1 < correlated_policies.size()) {
            ++state->comm;
            state->choice_count = 1;
            return POL_INIT_POL | POL_INIT_FWD;
        } else {
            state->violated = false;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
