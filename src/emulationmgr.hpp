#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "emulation.hpp"
#include "middlebox.hpp"
#include "pkt-hist.hpp"

/**
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
class EmulationMgr {
private:
    size_t _max_emu;                       // max number of emulations
    std::unordered_set<Emulation *> _emus; // all emulation instances
    std::unordered_map<Middlebox *,
                       std::map<NodePacketHistory *,
                                std::unordered_set<Emulation *>,
                                NodePacketHistoryComp>>
        _mb_emu_map;

    EmulationMgr();

public:
    // Disable the copy constructor and the copy assignment operator
    EmulationMgr(const EmulationMgr &)            = delete;
    EmulationMgr &operator=(const EmulationMgr &) = delete;
    ~EmulationMgr();

    static EmulationMgr &get();

    void reset();
    void max_emulations(decltype(_max_emu) n) { _max_emu = n; }

    Emulation *get_emulation(Middlebox *, NodePacketHistory *);
    void update_node_pkt_hist(Emulation *, NodePacketHistory *);
};
