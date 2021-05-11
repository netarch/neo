#include "policy/consistency.hpp"

#include "model-access.hpp"
#include "model.h"

std::string ConsistencyPolicy::to_string() const
{
    std::string ret = "Consistency of:\n";
    for (Policy *p : correlated_policies) {
        ret += p->to_string() + "\n";
    }
    return ret;
}

void ConsistencyPolicy::init(State *state, const Network *network)
{
    set_correlated_policy_idx(state, 0);
    Policy::init(state, network);
    unset = true;
}

void ConsistencyPolicy::reinit(State *state, const Network *network)
{
    Policy::init(state, network);
}

int ConsistencyPolicy::check_violation(State *state)
{
    correlated_policies[state->correlated_policy_idx]->check_violation(state);

    if (state->choice_count == 0) {
        Logger::info("Subpolicy " +
                     std::to_string(get_correlated_policy_idx(state)) +
                     (state->violated ? " violated!" : " verified"));

        // for the first execution path of the first subpolicy, store the
        // verification result
        if (unset) {
            result = state->violated;
            unset = false;
        }

        // check for consistency
        if (state->violated != result) {
            state->violated = true;
            state->choice_count = 0;
            return POL_NULL;
        }

        // next subpolicy
        if ((size_t)state->correlated_policy_idx + 1 < correlated_policies.size()) {
            ++state->correlated_policy_idx;
            return POL_REINIT_DP;
        } else {    // we have checked all the subpolicies
            state->violated = false;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
