#pragma once

#include <map>
#include <set>
#include <string>
#include <functional>

class FIB_IPNH;
class FIB;
#include "node.hpp"
#include "lib/hash.hpp"

class FIB_IPNH  // FIB entry for an IP next hop
{
private:
    Node *l3_node;      // L3 next hop node
    Interface *l3_intf; // L3 next hop interface
    Node *l2_node;      // L2 next hop node
    Interface *l2_intf; // L2 next hop interface
    /*
     * Note that l3_node and l2_node should/would never be nullptr, but l3_intf
     * and l2_intf can both be nullptrs (at the same time), which only occurs
     * when l3_node and l2_node are both the same.
     */

    friend struct std::hash<FIB_IPNH>;
    friend bool operator<(const FIB_IPNH&, const FIB_IPNH&);
    friend bool operator>(const FIB_IPNH&, const FIB_IPNH&);
    friend bool operator==(const FIB_IPNH&, const FIB_IPNH&);

public:
    FIB_IPNH(Node *l3nh, Interface *l3nh_intf,
             Node *l2nh, Interface *l2nh_intf);
    FIB_IPNH(const FIB_IPNH&) = default;
    FIB_IPNH(FIB_IPNH&&) = default;

    FIB_IPNH& operator=(FIB_IPNH&&) = default;

    std::string to_string() const;
    Node *const& get_l3_node() const;
    Interface *const& get_l3_intf() const;
    Node *const& get_l2_node() const;
    Interface *const& get_l2_intf() const;
};

bool operator<(const FIB_IPNH&, const FIB_IPNH&);
bool operator>(const FIB_IPNH&, const FIB_IPNH&);
bool operator==(const FIB_IPNH&, const FIB_IPNH&);

/*
 * An FIB holds the dataplane information for the current EC.
 */
class FIB
{
private:
    // resolved forwarding table for IP next hops
    std::map<Node *, std::set<FIB_IPNH> > tbl;
    // TODO: change tbl from map to unordered_map(hash table) ?
    // TODO: find a way to increase hashing speed for FIB when update agent is
    // implemented. (incremental hashing)

    friend struct std::hash<FIB>;
    friend bool operator==(const FIB&, const FIB&);

public:
    std::string to_string() const;
    void set_ipnhs(Node *, std::set<FIB_IPNH>&&);
    void add_ipnh(Node *, FIB_IPNH&&);
    const std::set<FIB_IPNH>& lookup(Node *const) const;
};

bool operator==(const FIB&, const FIB&);


namespace std
{

template <>
struct hash<FIB_IPNH> {
    size_t operator()(const FIB_IPNH& next_hop) const
    {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<Interface *> intf_hf;
        ::hash::hash_combine(value, node_hf(next_hop.l3_node));
        ::hash::hash_combine(value, intf_hf(next_hop.l3_intf));
        ::hash::hash_combine(value, node_hf(next_hop.l2_node));
        ::hash::hash_combine(value, intf_hf(next_hop.l2_intf));
        return value;
    }
};

template <>
struct hash<FIB> {
    size_t operator()(const FIB& fib) const
    {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<FIB_IPNH> ipnh_hf;
        for (const auto& entry : fib.tbl) {
            ::hash::hash_combine(value, node_hf(entry.first));
            for (const auto& nh : entry.second) {
                ::hash::hash_combine(value, ipnh_hf(nh));
            }
        }
        return value;
    }
};

template <>
struct hash<FIB *> {
    size_t operator()(FIB *const& fib) const
    {
        return hash<FIB>()(*fib);
    }
};

template <>
struct equal_to<FIB *> {
    bool operator()(FIB *const& a, FIB *const& b) const
    {
        return *a == *b;
    }
};

} // namespace std
