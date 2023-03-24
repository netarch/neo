#include "emulationmgr.hpp"

#include <cassert>

using namespace std;

EmulationMgr::EmulationMgr() : _max_emu(0) {}

EmulationMgr::~EmulationMgr() {
    for (Emulation *emulation : _emus) {
        delete emulation;
    }
}

EmulationMgr &EmulationMgr::get() {
    static EmulationMgr instance;
    return instance;
}

Emulation *EmulationMgr::get_emulation(Middlebox *mb, NodePacketHistory *nph) {
    auto mb_map = _mb_emu_map.find(mb);

    if (mb_map == _mb_emu_map.end()) { // no emulation
        if (_emus.size() < _max_emu) {
            // create a new emulation
            Emulation *emu = new Emulation();
            emu->init(mb);
            _emus.insert(emu);
            _mb_emu_map[mb][nullptr].insert(emu);
            return emu;
        } else {
            // get an existing emulation of another middlebox
            assert(!_emus.empty());
            Emulation *emu = *_emus.begin(); // TODO: LRU?
            Middlebox *old_mb = emu->mb();
            NodePacketHistory *old_nph = emu->node_pkt_hist();

            assert(_mb_emu_map[old_mb][old_nph].erase(emu) == 1);
            if (_mb_emu_map[old_mb][old_nph].empty()) {
                _mb_emu_map[old_mb].erase(old_nph);
                if (_mb_emu_map[old_mb].empty()) {
                    _mb_emu_map.erase(old_mb);
                }
            }
            emu->init(mb);
            _mb_emu_map[mb][nullptr].insert(emu);
            return emu;
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
        /**
         * Note that the current implementation will not use more emulations
         * than the number of middleboxes. TODO: think about duplication
         * strategy.
         */
    }
}

void EmulationMgr::update_node_pkt_hist(Emulation *emu,
                                        NodePacketHistory *nph) {
    Middlebox *mb = emu->mb();
    NodePacketHistory *old_nph = emu->node_pkt_hist();

    assert(_mb_emu_map[mb][old_nph].erase(emu) == 1);
    if (_mb_emu_map[mb][old_nph].empty()) {
        _mb_emu_map[mb].erase(old_nph);
    }
    _mb_emu_map[mb][nph].insert(emu);
    emu->node_pkt_hist(nph);
}
