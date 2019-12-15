#include "pkt-hist.hpp"

#include <typeinfo>

#include "middlebox.hpp"

NodePacketHistory::NodePacketHistory(Packet *p, NodePacketHistory *h)
    : last_pkt(p), past_hist(h)
{
}

bool operator==(const NodePacketHistory& a, const NodePacketHistory& b)
{
    return (a.last_pkt == b.last_pkt && a.past_hist == b.past_hist);
}

/******************************************************************************/

PacketHistory::PacketHistory(const Network& network)
{
    const std::map<std::string, Node *>& nodes = network.get_nodes();

    for (const auto& pair : nodes) {
        Node *node = pair.second;
        if (typeid(*node) == typeid(Middlebox)) {
            tbl.emplace(node, nullptr);
        }
    }
}

void PacketHistory::set_node_pkt_hist(Node *node, NodePacketHistory *nph)
{
    tbl[node] = nph;
}

NodePacketHistory *PacketHistory::get_node_pkt_hist(Node *node)
{
    return tbl.at(node);
}

bool operator==(const PacketHistory& a, const PacketHistory& b)
{
    return a.tbl == b.tbl;
}
