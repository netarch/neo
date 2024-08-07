#pragma once

#include <map>
#include <set>
#include <string>

class Node;
class Interface;

class FIB_IPNH // FIB entry for an IP next hop
{
private:
    Node *_l3_node;      // L3 next hop node
    Interface *_l3_intf; // L3 next hop interface
    Node *_l2_node;      // L2 next hop node
    Interface *_l2_intf; // L2 next hop interface
    /**
     * Note that _l3_node and _l2_node should/would never be nullptr, but
     * _l3_intf and _l2_intf can both be nullptrs (at the same time), which only
     * occurs when _l3_node and _l2_node are both the same.
     */

    friend class FIB_IPNH_Hash;
    friend bool operator<(const FIB_IPNH &, const FIB_IPNH &);
    friend bool operator>(const FIB_IPNH &, const FIB_IPNH &);
    friend bool operator==(const FIB_IPNH &, const FIB_IPNH &);

public:
    FIB_IPNH(Node *l3nh,
             Interface *l3nh_intf,
             Node *l2nh,
             Interface *l2nh_intf);
    FIB_IPNH(const FIB_IPNH &) = default;
    FIB_IPNH(FIB_IPNH &&) = default;

    FIB_IPNH &operator=(FIB_IPNH &&) = default;

    std::string to_string() const;
    Node *const &l3_node() const;
    Interface *const &l3_intf() const;
    Node *const &l2_node() const;
    Interface *const &l2_intf() const;
};

bool operator<(const FIB_IPNH &, const FIB_IPNH &);
bool operator>(const FIB_IPNH &, const FIB_IPNH &);
bool operator==(const FIB_IPNH &, const FIB_IPNH &);

class FIB_IPNH_Hash {
public:
    size_t operator()(const FIB_IPNH &) const;
};

/**
 * An FIB holds the dataplane information for the current EC.
 */
class FIB {
private:
    std::map<Node *, std::set<FIB_IPNH>> _fib;

    friend class FIBHash;
    friend class FIBEq;

public:
    std::string to_string() const;
    void set_ipnhs(Node *, std::set<FIB_IPNH> &&);
    void add_ipnh(Node *, FIB_IPNH &&);
    const std::set<FIB_IPNH> &lookup(Node *const) const;
};

class FIBHash {
public:
    size_t operator()(const FIB *const &) const;
};

class FIBEq {
public:
    bool operator()(const FIB *const &, const FIB *const &) const;
};
