#include "invariant/consistency.hpp"

#include "model-access.hpp"

using namespace std;

string Consistency::to_string() const {
    string ret = "Consistency of:\n";
    for (const auto &p : _correlated_invs) {
        ret += p->to_string() + "\n";
    }
    return ret;
}

void Consistency::init() {
    model.set_correlated_inv_idx(0);
    Invariant::init();
    unset = true;
}

void Consistency::reinit() {
    Invariant::init();
}

int Consistency::check_violation() {
    _correlated_invs[model.get_correlated_inv_idx()]->check_violation();

    if (model.get_choice_count() == 0) {
        logger.info("Sub-invariant " +
                    std::to_string(model.get_correlated_inv_idx()) +
                    (model.get_violated() ? " violated!" : " verified"));

        // for the first execution path of the first sub-invariant, store the
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
