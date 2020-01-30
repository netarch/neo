#include "policy/consistency.hpp"

#include "process/forwarding.hpp"
#include "model.h"

ConsistencyPolicy::ConsistencyPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net)
    : Policy(), first_run(true)
{
    parse_correlated_policies(config, net);
}

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

void ConsistencyPolicy::init(State *state) const
{
    correlated_policies[state->comm]->init(state);
}

void ConsistencyPolicy::check_violation(State *state)
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
            return;
        }

        // next subpolicy
        if ((size_t)state->comm + 1 < correlated_policies.size()) {
            ++state->comm;
            state->choice_count = 1;
        } else {
            state->violated = false;
            state->choice_count = 0;
        }
    }
}
