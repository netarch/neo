#include "invariant/conditional.hpp"

#include "model-access.hpp"

using namespace std;

string Conditional::to_string() const {
    string ret = "Conditional of:\n";
    for (const auto &p : _correlated_invs) {
        ret += p->to_string() + "\n";
    }
    return ret;
}

void Conditional::init() {
    model.set_correlated_inv_idx(0);
    Invariant::init();
}

void Conditional::reinit() {
    Invariant::init();
}

int Conditional::check_violation() {
    _correlated_invs[model.get_correlated_inv_idx()]->check_violation();

    if (model.get_choice_count() == 0) {
        if (model.get_correlated_inv_idx() == 0) {
            // invariant holds if the first sub-inv does not
            if (model.get_violated()) {
                model.set_violated(false);
                model.set_choice_count(0);
                return POL_NULL;
            }
        } else {
            // invariant violated if any subsequent sub-inv fails to hold
            if (model.get_violated()) {
                model.set_violated(true);
                model.set_choice_count(0);
                return POL_NULL;
            }
        }

        // next sub-invariant
        if ((size_t)model.get_correlated_inv_idx() + 1 <
            _correlated_invs.size()) {
            model.set_correlated_inv_idx(model.get_correlated_inv_idx() + 1);
            return POL_REINIT_DP;
        } else { // we have checked all the sub-invariants
            model.set_violated(false);
            model.set_choice_count(0);
        }
    }

    return POL_NULL;
}
