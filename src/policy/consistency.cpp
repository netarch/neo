#include "policy/consistency.hpp"

#include "model-access.hpp"

std::string ConsistencyPolicy::to_string() const {
    std::string ret = "Consistency of:\n";
    for (Policy *p : correlated_policies) {
        ret += p->to_string() + "\n";
    }
    return ret;
}

void ConsistencyPolicy::init(const Network *network) {
    model.set_correlated_policy_idx(0);
    Policy::init(network);
    unset = true;
}

void ConsistencyPolicy::reinit(const Network *network) {
    Policy::init(network);
}

int ConsistencyPolicy::check_violation() {
    correlated_policies[model.get_correlated_policy_idx()]->check_violation();

    if (model.get_choice_count() == 0) {
        Logger::info("Subpolicy " +
                     std::to_string(model.get_correlated_policy_idx()) +
                     (model.get_violated() ? " violated!" : " verified"));

        // for the first execution path of the first subpolicy, store the
        // verification result
        if (unset) {
            result = model.get_violated();
            unset = false;
        }

        // check for consistency
        if (model.get_violated() != result) {
            model.set_violated(true);
            model.set_choice_count(0);
            return POL_NULL;
        }

        // next subpolicy
        if ((size_t)model.get_correlated_policy_idx() + 1 <
            correlated_policies.size()) {
            model.set_correlated_policy_idx(model.get_correlated_policy_idx() +
                                            1);
            return POL_REINIT_DP;
        } else { // we have checked all the subpolicies
            model.set_violated(false);
            model.set_choice_count(0);
        }
    }

    return POL_NULL;
}
