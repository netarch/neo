#pragma once

#include <map>
#include <set>
#include <utility>
#include <string>
class FIB_L2DM;
class FIB_IPNH;
class FIB;
#include "node.hpp"
#include "lib/ip.hpp"


class FIB_L2DM  // FIB entry for an L2 domain
{
private:
    std::map<Node *, std::set<std::pair<Node *, Interface *> > > tbl;

    friend class std::hash<FIB_L2DM>;
    friend bool operator==(const FIB_L2DM&, const FIB_L2DM&);

public:
    void insert(Node *, const std::pair<Node *, Interface *>&);
};

bool operator==(const FIB_L2DM&, const FIB_L2DM&);

class FIB_IPNH  // FIB entry for an IP next hop
{
private:
    Node *l3_node;      // L3 next hop node
    Node *l2_node;      // L2 next hop node
    Interface *l2_intf; // L2 next hop interface

    friend class std::hash<FIB_IPNH>;
    friend bool operator<(const FIB_IPNH&, const FIB_IPNH&);
    friend bool operator>(const FIB_IPNH&, const FIB_IPNH&);
    friend bool operator==(const FIB_IPNH&, const FIB_IPNH&);

public:
    FIB_IPNH(Node *, Node *, Interface *);

    std::string to_string() const;

    //Node *get_l3nh() const;
    //Node *get_l2nh() const;
    //Interface *get_l2nh_intf() const;
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
    std::map<Node *, std::set<FIB_IPNH> > iptbl;
    // L2 domain mappings
    std::map<Interface *, FIB_L2DM *> l2tbl;

    friend class std::hash<FIB>;
    friend bool operator==(const FIB&, const FIB&);

public:
    std::string to_string() const;

    void set_ipnhs(Node *, std::set<FIB_IPNH>&&);
    void set_l2dm(Interface *, FIB_L2DM *);
    //const std::set<FIB_IPNH>& get_ipnhs(const Node *) const;
    //const FIB_L2DM *get_l2dm(const Interface *) const;
    bool in_l2dm(Interface *) const;
};

bool operator==(const FIB&, const FIB&);


namespace std
{

template <>
struct hash<FIB_L2DM> {
    size_t operator()(const FIB_L2DM& l2dm) const
    {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<Interface *> intf_hf;

        for (const auto& entry : l2dm.tbl) {
            value <<= node_hf(entry.first) & 1;
            for (const auto& nh : entry.second) {
                value ^= node_hf(nh.first) + intf_hf(nh.second);
            }
        }

        return value;
    };
};

template <>
struct hash<FIB_L2DM *> {
    size_t operator()(FIB_L2DM *const& l2dm) const
    {
        return hash<FIB_L2DM>()(*l2dm);
    };
};

template <>
struct equal_to<FIB_L2DM *> {
    bool operator()(FIB_L2DM *const& a, FIB_L2DM *const& b) const
    {
        return *a == *b;
    }
};

template <>
struct hash<FIB_IPNH> {
    size_t operator()(const FIB_IPNH& next_hop) const
    {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<Interface *> intf_hf;

        value ^= node_hf(next_hop.l3_node);
        value <<= 1;
        value ^= node_hf(next_hop.l2_node);
        value <<= 1;
        value ^= intf_hf(next_hop.l2_intf);

        return value;
    }
};

template <>
struct hash<FIB> {
    size_t operator()(const FIB& fib) const
    {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<Interface *> intf_hf;
        hash<FIB_IPNH> ipnh_hf;
        hash<FIB_L2DM *> l2dm_hf;

        size_t iptbl_value = 0;
        for (const auto& entry : fib.iptbl) {
            iptbl_value <<= node_hf(entry.first) & 1;
            for (const auto& nh : entry.second) {
                iptbl_value ^= ipnh_hf(nh);
            }
        }

        size_t l2tbl_value = 0;
        for (const auto& entry : fib.l2tbl) {
            l2tbl_value <<= intf_hf(entry.first) & 1;
            l2tbl_value ^= l2dm_hf(entry.second);
        }

        value = (iptbl_value + 1) ^ l2tbl_value;

        return value;
    };
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
