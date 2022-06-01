#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>

#include "lib/hash.hpp"
#include "lib/ip.hpp"
class Node;
class Interface;

class L2_LAN {
private:
    std::map<Node *, std::set<std::pair<Node *, Interface *>>> tbl;
    std::set<std::pair<Node *, Interface *>> l2_endpoints;
    std::map<IPv4Address, std::pair<Node *, Interface *>> l3_endpoints;

    friend struct std::hash<L2_LAN>;
    friend bool operator==(const L2_LAN &, const L2_LAN &);

    void collect_intfs(Node *, Interface *);

public:
    L2_LAN(Node *, Interface *);

    std::string to_string() const;
    const std::set<std::pair<Node *, Interface *>> &get_l2_endpoints() const;
    const std::map<IPv4Address, std::pair<Node *, Interface *>> &
    get_l3_endpoints() const;
    std::pair<Node *, Interface *> find_l3_endpoint(const IPv4Address &) const;
};

bool operator==(const L2_LAN &, const L2_LAN &);

namespace std {

template <> struct hash<L2_LAN> {
    size_t operator()(const L2_LAN &l2dm) const {
        size_t value = 0;
        hash<Node *> node_hf;
        hash<Interface *> intf_hf;
        for (const auto &entry : l2dm.tbl) {
            value <<= node_hf(entry.first) & 1;
            for (const auto &nh : entry.second) {
                ::hash::hash_combine(value, node_hf(nh.first));
                ::hash::hash_combine(value, intf_hf(nh.second));
            }
        }
        return value;
    };
};

template <> struct hash<L2_LAN *> {
    size_t operator()(L2_LAN *const &l2dm) const {
        return hash<L2_LAN>()(*l2dm);
    };
};

template <> struct equal_to<L2_LAN *> {
    bool operator()(L2_LAN *const &a, L2_LAN *const &b) const {
        return *a == *b;
    }
};

} // namespace std
