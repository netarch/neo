#include "pkt-hist.hpp"

#include <typeinfo>

#include "middlebox.hpp"

NodePacketHistory::NodePacketHistory(Packet *p, NodePacketHistory *h)
    : last_pkt(p), past_hist(h) {}

std::list<Packet *> NodePacketHistory::get_packets() const {
    std::list<Packet *> packets;
    for (const NodePacketHistory *nph = this; nph; nph = nph->past_hist) {
        packets.push_front(nph->last_pkt);
    }
    return packets;
}

std::list<Packet *>
NodePacketHistory::get_packets_since(NodePacketHistory *start) const {
    std::list<Packet *> packets;
    for (const NodePacketHistory *nph = this; nph && nph != start;
         nph = nph->past_hist) {
        packets.push_front(nph->last_pkt);
    }
    return packets;
}

bool NodePacketHistory::contains(NodePacketHistory *other) const {
    const NodePacketHistory *nph = this;
    while (nph && nph != other) {
        nph = nph->past_hist;
    }
    return nph == other;
}

bool operator==(const NodePacketHistory &a, const NodePacketHistory &b) {
    return (a.last_pkt == b.last_pkt && a.past_hist == b.past_hist);
}

/******************************************************************************/

PacketHistory::PacketHistory(const Network &network) {
    const std::map<std::string, Node *> &nodes = network.get_nodes();

    for (const auto &pair : nodes) {
        Node *node = pair.second;
        if (typeid(*node) == typeid(Middlebox)) {
            tbl.emplace(node, nullptr);
        }
    }
}

void PacketHistory::set_node_pkt_hist(Node *node, NodePacketHistory *nph) {
    tbl[node] = nph;
}

NodePacketHistory *PacketHistory::get_node_pkt_hist(Node *node) {
    return tbl.at(node);
}

bool operator==(const PacketHistory &a, const PacketHistory &b) {
    return a.tbl == b.tbl;
}

/******************************************************************************/

size_t NodePacketHistoryHash::operator()(NodePacketHistory *const &nph) const {
    if (nph == nullptr) {
        return 0;
    }

    size_t value = 0;
    std::hash<Packet *> pkt_hf;
    std::hash<NodePacketHistory *> nph_hf;
    ::hash::hash_combine(value, pkt_hf(nph->last_pkt));
    ::hash::hash_combine(value, nph_hf(nph->past_hist));
    return value;
}

bool NodePacketHistoryEq::operator()(NodePacketHistory *const &a,
                                     NodePacketHistory *const &b) const {
    if (a == b) {
        return true;
    } else if (a == nullptr || b == nullptr) {
        return false;
    }
    return *a == *b;
}

bool NodePacketHistoryComp::operator()(NodePacketHistory *const &a,
                                       NodePacketHistory *const &b) const {
    if (a == b) {
        return false;
    } else if (a == nullptr) {
        return true;
    } else if (b == nullptr) {
        return false;
    }

    std::list<Packet *> a_pkts = a->get_packets();
    std::list<Packet *> b_pkts = b->get_packets();

    for (auto a_it = a_pkts.begin(), b_it = b_pkts.begin();
         a_it != a_pkts.end() && b_it != b_pkts.end(); ++a_it, ++b_it) {
        if (*a_it != *b_it) {
            return *a_it < *b_it;
        }
    }

    return a_pkts.size() < b_pkts.size();
}

size_t PacketHistoryHash::operator()(PacketHistory *const &ph) const {
    size_t value = 0;
    std::hash<Node *> node_hf;
    std::hash<NodePacketHistory *> nph_hf;
    for (const auto &entry : ph->tbl) {
        ::hash::hash_combine(value, node_hf(entry.first));
        ::hash::hash_combine(value, nph_hf(entry.second));
    }
    return value;
}

bool PacketHistoryEq::operator()(PacketHistory *const &a,
                                 PacketHistory *const &b) const {
    return *a == *b;
}
