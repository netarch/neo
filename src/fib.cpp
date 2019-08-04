#include <unordered_set>

#include "fib.hpp"
#include "lib/logger.hpp"

void FIB_L2DM::insert(Node *node, const std::pair<Node *, Interface *>& peer)
{
    tbl[node].insert(peer);
}

std::string FIB_L2DM::to_string() const
{
    std::string ret;
    for (const auto& entry : tbl) {
        ret += entry.first->to_string() + " -> [";
        for (const auto& peer : entry.second) {
            ret += " (" + peer.first->to_string() + ", "
                   + peer.second->to_string() + ")";
        }
        ret += " ]\n";
    }
    return ret;
}

bool operator==(const FIB_L2DM& a, const FIB_L2DM& b)
{
    return a.tbl == b.tbl;
}

FIB_IPNH::FIB_IPNH(Node *l3nh, Node *l2nh, Interface *l2nh_intf)
    : l3_node(l3nh), l2_node(l2nh), l2_intf(l2nh_intf)
{
    if (!l3_node || !l2_node) {
        Logger::get_instance().err("Invalid IP next hop");
    }
}

std::string FIB_IPNH::to_string() const
{
    std::string ret;
    ret = "(" + l2_node->to_string() + "[" + l3_node->to_string() + "]";
    if (l2_intf) {
        ret += ", " + l2_intf->to_string();
    }
    ret += ")";
    return ret;
}

bool operator<(const FIB_IPNH& a, const FIB_IPNH& b)
{
    if (a.l3_node < b.l3_node) {
        return true;
    } else if (a.l3_node > b.l3_node) {
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
           && (a.l2_node == b.l2_node)
           && (a.l2_intf == b.l2_intf);
}

//Node *FIB_IPNH::get_l3nh() const
//{
//    return l3_node;
//}
//
//Node *FIB_IPNH::get_l2nh() const
//{
//    return l2_node;
//}
//
//Interface *FIB_IPNH::get_l2nh_intf() const
//{
//    return l2_intf;
//}

std::string FIB::to_string() const
{
    std::string ret = "FIB:\n";
    ret += "IP next hops:\n";
    for (const auto& entry : iptbl) {
        ret += entry.first->to_string() + " -> [";
        for (auto it = entry.second.begin(); it != entry.second.end(); ++it) {
            ret += " " + it->to_string();
        }
        ret += " ]\n";
    }
    ret += "L2 domains:\n";
    std::unordered_set<FIB_L2DM *> printed_l2dms;
    for (const auto& entry : l2tbl) {
        if (printed_l2dms.count(entry.second) == 0) {
            ret += entry.second->to_string();
            printed_l2dms.insert(entry.second);
        }
    }
    return ret;
}

void FIB::set_ipnhs(Node *node, std::set<FIB_IPNH>&& next_hops)
{
    iptbl[node] = next_hops;
}

void FIB::set_l2dm(Interface *intf, FIB_L2DM *l2dm)
{
    l2tbl[intf] = l2dm;
}

//const std::set<FIB_IPNH>& FIB::get_ipnhs(const Node *node) const
//{
//    return iptbl.at(node);
//}
//
//const FIB_L2DM *FIB::get_l2dm(const Interface *intf) const
//{
//    return l2tbl.at(intf);
//}

bool FIB::in_l2dm(Interface *intf) const
{
    return l2tbl.count(intf) > 0;
}

bool operator==(const FIB& a, const FIB& b)
{
    return (a.iptbl == b.iptbl) && (a.l2tbl == b.l2tbl);
}
