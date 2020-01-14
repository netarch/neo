#include "policy/consistency.hpp"

#include "process/forwarding.hpp"
#include "model.h"

ConsistencyPolicy::ConsistencyPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net)
    : Policy()
{
    parse_correlated_policies(config, net);
}

std::string ConsistencyPolicy::to_string() const
{
    std::string ret = "consistency of:";
    for (Policy *p : correlated_policies) {
        ret += "\n" + p->to_string();
    }
    return ret;
}

void ConsistencyPolicy::init(State *state) const
{
    correlated_policies[state->comm]->init(state);
}

void ConsistencyPolicy::check_violation(State *state)
{
    correlated_policies[state->comm]->check_violation(state);

    if (state->choice_count == 0) {
        // for the first policy, store the verification result
        if (state->comm == 0) {
            subpolicy_violated = state->violated;
        }

        // check for consistency
        if (state->violated != subpolicy_violated) {
            state->violated = true;
            state->choice_count = 0;
            return;
        }

        // next subpolicy
        if (++state->comm == correlated_policies.size()) {
            state->violated = false;
            state->choice_count = 0;
            return;
        }
        state->choice_count = 1;
    }
}
