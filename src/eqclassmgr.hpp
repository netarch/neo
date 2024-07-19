#pragma once

#include <set>

#include "eqclass.hpp"
#include "lib/ip.hpp"

class Network;
class OpenflowProcess;

class EqClassMgr {
private:
    std::set<ECRange> _allranges;
    std::set<EqClass *> _all_ecs, _owned_ecs;
    std::set<uint16_t> _ports;

    EqClassMgr() = default;
    void split_intersected_ec(EqClass *ec, const ECRange &range, bool owned);
    void add_non_overlapped_ec(const ECRange &, bool owned);
    void add_ec(const ECRange &, bool owned);

public:
    // Disable the copy constructor and the copy assignment operator
    EqClassMgr(const EqClassMgr &) = delete;
    EqClassMgr &operator=(const EqClassMgr &) = delete;
    ~EqClassMgr();

    static EqClassMgr &get();

    const decltype(_all_ecs) &all_ecs() const { return _all_ecs; }
    const decltype(_ports) &ports() const { return _ports; }

    void reset();
    void add_ec(const IPNetwork<IPv4Address> &);
    void add_ec(const IPv4Address &, bool owned = false);
    void compute_initial_ecs(const Network &, const OpenflowProcess &);

    std::set<EqClass *> get_overlapped_ecs(const ECRange &,
                                           bool owned_only = false) const;
    EqClass *find_ec(const IPv4Address &) const;
};
