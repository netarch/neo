#include "l2-lan.hpp"

#include "lib/logger.hpp"

void L2_LAN::collect_intfs(Node *node,
                           Interface *interface __attribute__((unused)))
{
    for (Interface *intf : node->get_intfs_l2()) {
        auto l2_endpoint = std::make_pair(node, intf);
        if (l2_endpoints.count(l2_endpoint) == 0) {
            l2_endpoints.insert(l2_endpoint);
            auto peer = node->get_peer(intf->get_name());
            if (peer.first) { // if the interface is truly connected (has peer)
                tbl[node].insert(peer);
                if (!peer.second->is_l2()) { // L3 interface
                    l3_endpoints[peer.second->addr()] = peer;
                } else if (l2_endpoints.count(peer) == 0) {
                    collect_intfs(peer.first, peer.second);
                }
            }
        }
    }
}

L2_LAN::L2_LAN(Node *node, Interface *interface)
{
    collect_intfs(node, interface);
}

std::string L2_LAN::to_string() const
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

const std::set<std::pair<Node *, Interface *> >&
L2_LAN::get_l2_endpoints() const
{
    return l2_endpoints;
}

const std::map<IPv4Address, std::pair<Node *, Interface *> >&
L2_LAN::get_l3_endpoints() const
{
    return l3_endpoints;
}

std::pair<Node *, Interface *>
L2_LAN::find_l3_endpoint(const IPv4Address& dst) const
{
    auto l3_peer = l3_endpoints.find(dst);
    if (l3_peer == l3_endpoints.end()) {
        return std::make_pair(nullptr, nullptr);
    }
    return l3_peer->second;
}

bool operator==(const L2_LAN& a, const L2_LAN& b)
{
    return a.tbl == b.tbl;
}
