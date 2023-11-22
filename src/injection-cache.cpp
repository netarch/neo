#include "injection-cache.hpp"

using namespace std;

InjectionCache &injection_cache = InjectionCache::_get();

InjectionCache &InjectionCache::_get() {
    static InjectionCache instance;
    return instance;
}

void InjectionCache::insert(Middlebox *mb,
                            NodePacketHistory *nph,
                            InjectionResults *results) {
    _cache[mb][nph] = results;
}

InjectionResults *InjectionCache::get(Middlebox *mb, NodePacketHistory *nph) {
    auto mb_it = _cache.find(mb);
    if (mb_it == _cache.end()) {
        return nullptr;
    }

    auto nph_it = mb_it->second.find(nph);
    if (nph_it == mb_it->second.end()) {
        return nullptr;
    }

    return nph_it->second;
}
