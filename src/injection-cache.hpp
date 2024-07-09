#pragma once

#include <unordered_map>

#include "injection-result.hpp"
#include "middlebox.hpp"
#include "pkt-hist.hpp"

class InjectionCache {
private:
    std::unordered_map<
        Middlebox *,
        std::unordered_map<NodePacketHistory *, InjectionResults *>>
        _cache;

    InjectionCache() = default;

public:
    // Disable the copy constructor and the copy assignment operator
    InjectionCache(const InjectionCache &)            = delete;
    InjectionCache &operator=(const InjectionCache &) = delete;

    static InjectionCache &_get();

    void insert(Middlebox *, NodePacketHistory *, InjectionResults *);
    InjectionResults *get(Middlebox *, NodePacketHistory *);
};

extern InjectionCache &injection_cache;
