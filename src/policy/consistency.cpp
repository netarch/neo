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

void ConsistencyPolicy::init(State *state, const Network *network) const
{
    set_correlated_policy_idx(state, 0);
    correlated_policies[get_correlated_policy_idx(state)]->init(state, network);
}

void ConsistencyPolicy::reinit(State *state, const Network *network) const
{
    correlated_policies[get_correlated_policy_idx(state)]->init(state, network);
}

int ConsistencyPolicy::check_violation(State *state)
{
    correlated_policies[state->correlated_policy_idx]->check_violation(state);

    if (state->choice_count == 0) {
        // for the first subpolicy, store the verification result
        if (state->correlated_policy_idx == 0) {
            result = state->violated;
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
