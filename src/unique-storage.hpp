#pragma once

#include <unordered_set>

#include "candidates.hpp"
#include "choices.hpp"
#include "fib.hpp"
#include "injection-result.hpp"
#include "invariant/loop.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
#include "process/openflow.hpp"
#include "reachcounts.hpp"

class UniqueStorage {
private:
    UniqueStorage() = default;
    friend class Plankton;
    void reset();

    // Variables of unique storage for preventing duplicates
    std::unordered_set<Candidates *, CandHash, CandEq> candidates_store;
    std::unordered_set<FIB *, FIBHash, FIBEq> fib_store;
    std::unordered_set<Choices *, ChoicesHash, ChoicesEq> choices_store;
    // All sent packets
    std::unordered_set<Packet *, PacketPtrHash, PacketPtrEq> pkt_store;
    std::unordered_set<NodePacketHistory *,
                       NodePacketHistoryHash,
                       NodePacketHistoryEq>
        node_pkt_hist_store;
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq>
        pkt_hist_store;
    std::
        unordered_set<OpenflowUpdateState *, OFUpdateStateHash, OFUpdateStateEq>
            openflow_update_state_store;
    std::unordered_set<ReachCounts *, ReachCountsHash, ReachCountsEq>
        reach_counts_store;
    std::
        unordered_set<InjectionResult *, InjectionResultHash, InjectionResultEq>
            injection_result_store;
    std::unordered_set<InjectionResults *,
                       InjectionResultsHash,
                       InjectionResultsEq>
        injection_results_store;
    std::unordered_set<VisitedHops *, VisitedHopsHash, VisitedHopsEq>
        visited_hops_store;

public:
    // Disable the copy constructor and the copy assignment operator
    UniqueStorage(const UniqueStorage &) = delete;
    UniqueStorage &operator=(const UniqueStorage &) = delete;
    ~UniqueStorage() { reset(); }

    static UniqueStorage &get();

    Candidates *store_candidates(Candidates *);
    FIB *store_fib(FIB *);
    Choices *store_choices(Choices *);
    Packet *store_packet(Packet *);
    NodePacketHistory *store_node_pkt_hist(NodePacketHistory *);
    PacketHistory *store_pkt_hist(PacketHistory *);
    OpenflowUpdateState *store_of_update_state(OpenflowUpdateState *);
    ReachCounts *store_reach_counts(ReachCounts *);
    InjectionResult *store_injection_result(InjectionResult *);
    InjectionResults *store_injection_results(InjectionResults *);
    VisitedHops *store_visited_hops(VisitedHops *);
};

extern UniqueStorage &storage;
