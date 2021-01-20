#include "fib.hpp"

#include "lib/logger.hpp"

FIB_IPNH::FIB_IPNH(Node *l3nh, Interface *l3nh_intf,
                   Node *l2nh, Interface *l2nh_intf)
    : l3_node(l3nh), l3_intf(l3nh_intf), l2_node(l2nh), l2_intf(l2nh_intf)
{
    if (!l3_node || !l2_node) {
        Logger::error("Invalid IP next hop");
    }
}

std::string FIB_IPNH::to_string() const
{
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

Node *const& FIB_IPNH::get_l3_node() const
{
    return l3_node;
}

Interface *const& FIB_IPNH::get_l3_intf() const
{
    return l3_intf;
}

Node *const& FIB_IPNH::get_l2_node() const
{
    return l2_node;
}

Interface *const& FIB_IPNH::get_l2_intf() const
{
    return l2_intf;
}

bool operator<(const FIB_IPNH& a, const FIB_IPNH& b)
{
    /*
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

bool operator>(const FIB_IPNH& a, const FIB_IPNH& b)
{
    return b < a;
}

bool operator==(const FIB_IPNH& a, const FIB_IPNH& b)
{
    return (a.l3_node == b.l3_node)
           && (a.l3_intf == b.l3_intf)
           && (a.l2_node == b.l2_node)
           && (a.l2_intf == b.l2_intf);
}

std::string FIB::to_string() const
{
    std::string ret = "FIB:\n";
    for (const auto& entry : tbl) {
        ret += entry.first->to_string() + " -> [";
        for (auto it = entry.second.begin(); it != entry.second.end(); ++it) {
            ret += " " + it->to_string();
        }
        ret += " ]\n";
    }
    return ret;
}

void FIB::set_ipnhs(Node *node, std::set<FIB_IPNH>&& next_hops)
{
    tbl[node] = next_hops;
}

void FIB::add_ipnh(Node *node, FIB_IPNH&& next_hop)
{
    tbl[node].insert(next_hop);
}

const std::set<FIB_IPNH>& FIB::lookup(Node *const node) const
{
    return tbl.at(node);
}

bool operator==(const FIB& a, const FIB& b)
{
    return a.tbl == b.tbl;
}
