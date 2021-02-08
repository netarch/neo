#include "emulationmgr.hpp"

#include <cassert>

EmulationMgr::EmulationMgr(): max_emulations(0)
{
}

EmulationMgr::~EmulationMgr()
{
    for (Emulation *emulation : emulations) {
        delete emulation;
    }
}

EmulationMgr& EmulationMgr::get()
{
    static EmulationMgr instance;
    return instance;
}

void EmulationMgr::set_max_emulations(size_t val)
{
    max_emulations = val;
}

void EmulationMgr::set_num_middleboxes(size_t val)
{
    num_middleboxes = val;
}

Emulation *EmulationMgr::get_emulation(Middlebox *mb, NodePacketHistory *nph)
{
    auto mb_map = mb_emulations_map.find(mb);

    if (mb_map == mb_emulations_map.end()) { // no emulation of this middlebox found
        if (emulations.size() < max_emulations) {
            // create a new emulation
            Emulation *emulation = new Emulation();
            emulation->init(mb);
            emulations.insert(emulation);
            mb_emulations_map[mb][nullptr].insert(emulation);
            return emulation;
        } else {
            // get an existing emulation of other middleboxes
            assert(!emulations.empty());
            Emulation *emulation = *emulations.begin(); // TODO: LRU?
            Middlebox *old_mb = emulation->get_mb();
            NodePacketHistory *old_nph = emulation->get_node_pkt_hist();

            assert(mb_emulations_map[old_mb][old_nph].erase(emulation) == 1);
            if (mb_emulations_map[old_mb][old_nph].empty()) {
                mb_emulations_map[old_mb].erase(old_nph);
                if (mb_emulations_map[old_mb].empty()) {
                    mb_emulations_map.erase(old_mb);
                }
            }
            emulation->init(mb);
            mb_emulations_map[mb][nullptr].insert(emulation);
            return emulation;
        }
    } else {
        // get an existing emulation of this middlebox
        auto res = mb_map->second.equal_range(nph);
        if (res.first != res.second) {
            return *(res.first->second.begin());
        }
        auto nph_map_itr = --res.first;
        while (nph_map_itr != mb_map->second.begin() &&
                !nph->contains(nph_map_itr->first)) {
            --nph_map_itr;
        }
        return *(nph_map_itr->second.begin());
        /*
         * Note that the current implementation will not use more emulations
         * than the number of middleboxes. TODO: think about duplication
         * strategy.
         */
    }
}

void EmulationMgr::update_node_pkt_hist(Emulation *emulation, NodePacketHistory *nph)
{
    Middlebox *mb = emulation->get_mb();
    NodePacketHistory *old_nph = emulation->get_node_pkt_hist();

    assert(mb_emulations_map[mb][old_nph].erase(emulation) == 1);
    if (mb_emulations_map[mb][old_nph].empty()) {
        mb_emulations_map[mb].erase(old_nph);
    }
    mb_emulations_map[mb][nph].insert(emulation);
    emulation->node_pkt_hist = nph;
}
