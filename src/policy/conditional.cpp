#include "policy/conditional.hpp"

#include "model-access.hpp"
#include "model.h"

std::string ConditionalPolicy::to_string() const
{
    std::string ret = "conditional policy of ";
    for (Policy *p : correlated_policies) {
        ret += p->to_string() + ", ";
    }
    ret.pop_back();
    ret.pop_back();
    return ret;
}

void ConditionalPolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, false);
    set_correlated_policy_idx(state, 0);
    correlated_policies[0]->init(state, network);
}

int ConditionalPolicy::check_violation(State *state)
{
    correlated_policies[state->correlated_policy_idx]->check_violation(state);

    if (state->choice_count == 0) {
        if (state->correlated_policy_idx == 0) {
            // policy holds if the first subpolicy does not
            if (state->violated) {
                state->violated = false;
                state->choice_count = 0;
                return POL_NULL;
            }
        } else {
            // policy violated if any subsequent subpolicy fails to hold
            if (state->violated) {
                state->violated = true;
                state->choice_count = 0;
                return POL_NULL;
            }
        }

        // next subpolicy
        if ((size_t)state->correlated_policy_idx + 1 < correlated_policies.size()) {
            ++state->correlated_policy_idx;
            state->choice_count = 1;
            correlated_policies[state->correlated_policy_idx]->init(state);
            return POL_INIT_FWD;
        } else {    // we have checked all the subpolicies
            state->violated = false;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
