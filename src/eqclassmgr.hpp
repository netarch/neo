#pragma once

#include <set>

#include "eqclass.hpp"
#include "lib/ip.hpp"
class Network;
class OpenflowProcess;

class EqClassMgr
{
private:
    std::set<ECRange> allranges;
    std::set<EqClass *> all_ECs, owned_ECs;
    std::set<uint16_t> ports;

    EqClassMgr() = default;
    void split_intersected_ec(EqClass *ec, const ECRange& range, bool owned);
    void add_non_overlapped_ec(const ECRange&, bool owned);

public:
    // Disable the copy constructor and the copy assignment operator
    EqClassMgr(const EqClassMgr&) = delete;
    EqClassMgr& operator=(const EqClassMgr&) = delete;
    ~EqClassMgr();

    static EqClassMgr& get();

    void add_ec(const ECRange&, bool owned);
    void add_ec(const IPNetwork<IPv4Address>&);
    void add_ec(const IPv4Address&, bool owned = false);
    void compute_policy_oblivious_ecs(const Network&, const OpenflowProcess&);

    std::set<EqClass *> get_overlapped_ecs(const ECRange&, bool owned_only = false) const;
    const std::set<uint16_t>& get_ports() const;
    EqClass *find_ec(const IPv4Address&) const;
};
