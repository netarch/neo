#include "unique-storage.hpp"

using namespace std;

UniqueStorage &storage = UniqueStorage::get();

UniqueStorage &UniqueStorage::get() {
    static UniqueStorage instance;
    return instance;
}

void UniqueStorage::reset() {
    for (Candidates *candidates : this->candidates_store) {
        delete candidates;
    }
    this->candidates_store.clear();

    for (FIB *fib : this->fib_store) {
        delete fib;
    }
    this->fib_store.clear();

    for (Choices *path_choices : this->choices_store) {
        delete path_choices;
    }
    this->choices_store.clear();

    for (PacketHistory *pkt_hist : this->pkt_hist_store) {
        delete pkt_hist;
    }
    this->pkt_hist_store.clear();

    for (OpenflowUpdateState *update_state :
         this->openflow_update_state_store) {
        delete update_state;
    }
    this->openflow_update_state_store.clear();

    for (ReachCounts *reach_counts : this->reach_counts_store) {
        delete reach_counts;
    }
    this->reach_counts_store.clear();

    for (InjectionResult *result : this->injection_result_store) {
        delete result;
    }
    this->injection_result_store.clear();

    for (InjectionResults *results : this->injection_results_store) {
        delete results;
    }
    this->injection_results_store.clear();
}

Candidates *UniqueStorage::store_candidates(Candidates *candidates) {
    auto res = storage.candidates_store.insert(candidates);
    if (!res.second) {
        delete candidates;
        candidates = *(res.first);
    }
    return candidates;
}

FIB *UniqueStorage::store_fib(FIB *fib) {
    auto res = storage.fib_store.insert(fib);
    if (!res.second) {
        delete fib;
        fib = *(res.first);
    }
    return fib;
}

Choices *UniqueStorage::store_choices(Choices *choices) {
    auto res = storage.choices_store.insert(choices);
    if (!res.second) {
        delete choices;
        choices = *(res.first);
    }
    return choices;
}

PacketHistory *UniqueStorage::store_pkt_hist(PacketHistory *pkt_hist) {
    auto res = storage.pkt_hist_store.insert(pkt_hist);
    if (!res.second) {
        delete pkt_hist;
        pkt_hist = *(res.first);
    }
    return pkt_hist;
}

OpenflowUpdateState *
UniqueStorage::store_of_update_state(OpenflowUpdateState *update_state) {
    auto res = storage.openflow_update_state_store.insert(update_state);
    if (!res.second) {
        delete update_state;
        update_state = *(res.first);
    }
    return update_state;
}

ReachCounts *UniqueStorage::store_reach_counts(ReachCounts *reach_counts) {
    auto res = storage.reach_counts_store.insert(reach_counts);
    if (!res.second) {
        delete reach_counts;
        reach_counts = *(res.first);
    }
    return reach_counts;
}

InjectionResult *
UniqueStorage::store_injection_result(InjectionResult *result) {
    auto res = storage.injection_result_store.insert(result);
    if (!res.second) {
        delete result;
        result = *(res.first);
    }
    return result;
}

InjectionResults *
UniqueStorage::store_injection_results(InjectionResults *results) {
    auto res = storage.injection_results_store.insert(results);
    if (!res.second) {
        delete results;
        results = *(res.first);
    }
    return results;
}
