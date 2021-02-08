#pragma once

#include <unordered_set>
#include <unordered_map>
#include <map>

#include "emulation.hpp"
#include "middlebox.hpp"
#include "pkt-hist.hpp"

/*
 * Class for managing the pool of emulations.
 * The emulations are indexed firstly by their emulating middlebox nodes and
 * secondly by the packet histories in the alphabetical order. For example:
 * (pktA)
 * (pktA, pktA)
 * (pktA, pktB)
 * (pktA, pktB, pktC)
 * (pktA, pktB, pktD)
 * (pktA, pktC)
 * (pktB)
 * (pktB, pktA)
 * (pktC, pktE)
 */
class EmulationMgr
{
private:
    size_t max_emulations;
    size_t num_middleboxes;
    std::unordered_set<Emulation *> emulations;  // all emulation instances
    std::unordered_map<Middlebox *,
        std::map<NodePacketHistory *, std::unordered_set<Emulation *>, NodePacketHistoryComp>
        > mb_emulations_map;

    EmulationMgr();

public:
    // Disable the copy constructor and the copy assignment operator
    EmulationMgr(const EmulationMgr&) = delete;
    EmulationMgr& operator=(const EmulationMgr&) = delete;
    ~EmulationMgr();

    static EmulationMgr& get();

    void set_max_emulations(size_t);
    void set_num_middleboxes(size_t);
    Emulation *get_emulation(Middlebox *, NodePacketHistory *);
    void update_node_pkt_hist(Emulation *, NodePacketHistory *);
};
