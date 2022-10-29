#include "policy/conditional.hpp"

#include "model-access.hpp"

std::string ConditionalPolicy::to_string() const {
    std::string ret = "Conditional policy of:\n";
    for (Policy *p : correlated_policies) {
        ret += p->to_string() + "\n";
    }
    return ret;
}

void ConditionalPolicy::init(const Network *network) {
    model.set_correlated_policy_idx(0);
    Policy::init(network);
}

void ConditionalPolicy::reinit(const Network *network) {
    Policy::init(network);
}

int ConditionalPolicy::check_violation() {
    correlated_policies[model.get_correlated_policy_idx()]->check_violation();

    if (model.get_choice_count() == 0) {
        if (model.get_correlated_policy_idx() == 0) {
            // policy holds if the first subpolicy does not
            if (model.get_violated()) {
                model.set_violated(false);
                model.set_choice_count(0);
                return POL_NULL;
            }
        } else {
            // policy violated if any subsequent subpolicy fails to hold
            if (model.get_violated()) {
                model.set_violated(true);
                model.set_choice_count(0);
                return POL_NULL;
            }
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
