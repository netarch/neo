#include "fib.hpp"

#include "interface.hpp"
#include "lib/hash.hpp"
#include "logger.hpp"
#include "node.hpp"

FIB_IPNH::FIB_IPNH(Node *l3nh,
                   Interface *l3nh_intf,
                   Node *l2nh,
                   Interface *l2nh_intf)
    : l3_node(l3nh), l3_intf(l3nh_intf), l2_node(l2nh), l2_intf(l2nh_intf) {}

std::string FIB_IPNH::to_string() const {
    std::string ret;
    ret = "(" + l3_node->to_string();
    if (l3_intf) {
        ret += ", " + l3_intf->to_string();
    }
    ret += ")-[" + l2_node->to_string();
    if (l2_intf) {
        ret += ", " + l2_intf->to_string();
    }
    ret += "]";
    return ret;
}

Node *const &FIB_IPNH::get_l3_node() const {
    return l3_node;
}

Interface *const &FIB_IPNH::get_l3_intf() const {
    return l3_intf;
}

Node *const &FIB_IPNH::get_l2_node() const {
    return l2_node;
}

Interface *const &FIB_IPNH::get_l2_intf() const {
    return l2_intf;
}

bool operator<(const FIB_IPNH &a, const FIB_IPNH &b) {
    /**
     * NOTE:
     * It's important that l3_node is the most significant element, since the
     * candidates (vector of L3 nodes) are derived from the set of FIB_IPNHs. By
     * making them in order, we eliminate potential duplicate vectors of
     * candidates with different orderings.
     */
    if (a.l3_node < b.l3_node) {
        return true;
    } else if (a.l3_node > b.l3_node) {
        return false;
    }
    if (a.l3_intf < b.l3_intf) {
        return true;
    } else if (a.l3_intf > b.l3_intf) {
        return false;
    }
    if (a.l2_node < b.l2_node) {
        return true;
    } else if (a.l2_node > b.l2_node) {
        return false;
    }
    if (a.l2_intf < b.l2_intf) {
        return true;
    }
    return false;
}

bool operator>(const FIB_IPNH &a, const FIB_IPNH &b) {
    return b < a;
}

bool operator==(const FIB_IPNH &a, const FIB_IPNH &b) {
    return (a.l3_node == b.l3_node) && (a.l3_intf == b.l3_intf) &&
           (a.l2_node == b.l2_node) && (a.l2_intf == b.l2_intf);
}

size_t FIB_IPNH_Hash::operator()(const FIB_IPNH &next_hop) const {
    size_t value = 0;
    std::hash<Node *> node_hf;
    std::hash<Interface *> intf_hf;
    hash::hash_combine(value, node_hf(next_hop.l3_node));
    hash::hash_combine(value, intf_hf(next_hop.l3_intf));
    hash::hash_combine(value, node_hf(next_hop.l2_node));
    hash::hash_combine(value, intf_hf(next_hop.l2_intf));
    return value;
}

std::string FIB::to_string() const {
    std::string ret = "FIB:\n";
    for (const auto &entry : tbl) {
        ret += entry.first->to_string() + " -> [";
        for (auto it = entry.second.begin(); it != entry.second.end(); ++it) {
            ret += " " + it->to_string();
        }
        ret += " ]\n";
    }
    return ret;
}

void FIB::set_ipnhs(Node *node, std::set<FIB_IPNH> &&next_hops) {
    tbl[node] = next_hops;
}

void FIB::add_ipnh(Node *node, FIB_IPNH &&next_hop) {
    tbl[node].insert(next_hop);
}

const std::set<FIB_IPNH> &FIB::lookup(Node *const node) const {
    return tbl.at(node);
}

size_t FIBHash::operator()(const FIB *const &fib) const {
    size_t value = 0;
    std::hash<Node *> node_hf;
    FIB_IPNH_Hash ipnh_hf;
    for (const auto &entry : fib->tbl) {
        hash::hash_combine(value, node_hf(entry.first));
        for (const auto &nh : entry.second) {
            hash::hash_combine(value, ipnh_hf(nh));
        }
    }
    return value;
}

bool FIBEq::operator()(const FIB *const &a, const FIB *const &b) const {
    return a->tbl == b->tbl;
}
