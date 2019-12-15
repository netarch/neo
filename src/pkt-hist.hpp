#pragma once

#include <unordered_map>

#include "packet.hpp"
#include "node.hpp"
#include "network.hpp"
#include "lib/hash.hpp"

/*
 * NodePacketHistory contains the packet traversal history of *one* node, which
 * is represented as the "state" of that node.
 *
 * A (NodePacketHistory *) being null means empty history.
 */
class NodePacketHistory
{
private:
    Packet *last_pkt;
    NodePacketHistory *past_hist;

    friend struct NodePacketHistoryHash;
    friend bool operator==(const NodePacketHistory&, const NodePacketHistory&);

public:
    NodePacketHistory(Packet *, NodePacketHistory *);
};

bool operator==(const NodePacketHistory&, const NodePacketHistory&);

/*
 * PacketHistory contains the packet traversal history (of each node) for the
 * whole network and the current EC.
 */
class PacketHistory
{
private:
    std::unordered_map<Node *, NodePacketHistory *> tbl;

    friend struct PacketHistoryHash;
    friend bool operator==(const PacketHistory&, const PacketHistory&);

public:
    PacketHistory(const Network&);
    PacketHistory(const PacketHistory&) = default;

    void set_node_pkt_hist(Node *, NodePacketHistory *);
    NodePacketHistory *get_node_pkt_hist(Node *);
};

bool operator==(const PacketHistory&, const PacketHistory&);

struct NodePacketHistoryHash {
    size_t operator()(NodePacketHistory *const& nph) const
    {
        size_t value = 0;
        std::hash<Packet *> pkt_hf;
        std::hash<NodePacketHistory *> nph_hf;
        ::hash::hash_combine(value, pkt_hf(nph->last_pkt));
        ::hash::hash_combine(value, nph_hf(nph->past_hist));
        return value;
    }
};

struct NodePacketHistoryEq {
    bool operator()(NodePacketHistory *const& a, NodePacketHistory *const& b)
    const
    {
        return *a == *b;
    }
};

struct PacketHistoryHash {
    size_t operator()(PacketHistory *const& ph) const
    {
        size_t value = 0;
        std::hash<Node *> node_hf;
        std::hash<NodePacketHistory *> nph_hf;
        for (const auto& entry : ph->tbl) {
            ::hash::hash_combine(value, node_hf(entry.first));
            ::hash::hash_combine(value, nph_hf(entry.second));
        }
        return value;
    }
};

struct PacketHistoryEq {
    bool operator()(PacketHistory *const& a, PacketHistory *const& b) const
    {
        return *a == *b;
    }
};
