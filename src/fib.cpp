#include "fib.hpp"
#include "lib/logger.hpp"

void FIB_L2DM::collect_intfs(Node *node,
                             Interface *interface __attribute__((unused)))
{
    for (Interface *intf : node->get_intfs_l2()) {
        if (intfs.count(intf) == 0) {
            intfs.insert(intf);
            auto peer = node->get_peer(intf->get_name());
            if (peer.first) {
                // if the interface is truly connected (has peer)
                tbl[node].insert(peer);
                if (intfs.count(peer.second) == 0) {
                    collect_intfs(peer.first, peer.second);
                }
            }
        }
    }
}

FIB_L2DM::FIB_L2DM(Node *node, Interface *interface)
{
    collect_intfs(node, interface);
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

Node *const& FIB_IPNH::get_l3_node() const
{
    return l3_node;
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

void FIB::set_l2dm(FIB_L2DM *l2dm)
{
    for (const auto& entry : l2dm->tbl) {
        for (const auto& peer : entry.second) {
            l2tbl[peer.second] = l2dm;
        }
    }
}

bool FIB::in_l2dm(Interface *intf) const
{
    return l2tbl.count(intf) > 0;
}

const std::set<FIB_IPNH>& FIB::lookup(Node *const node) const
{
    return iptbl.at(node);
}

FIB_L2DM *const& FIB::lookup(Interface *const intf) const
{
    return l2tbl.at(intf);
}

bool operator==(const FIB& a, const FIB& b)
{
    return (a.iptbl == b.iptbl) && (a.l2tbl == b.l2tbl);
}
