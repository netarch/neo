#pragma once

#include <list>
#include <unordered_map>

#include "lib/hash.hpp"
#include "network.hpp"
#include "node.hpp"
#include "packet.hpp"

/**
 * NodePacketHistory contains the packet traversal history of one node, which is
 * represented as the "state" of that node.
 *
 * A (NodePacketHistory *) being null means empty history.
 */
class NodePacketHistory {
private:
    Packet *last_pkt;
    NodePacketHistory *past_hist;

    friend struct NodePacketHistoryHash;
    friend struct NodePacketHistoryComp;
    friend bool operator==(const NodePacketHistory &,
                           const NodePacketHistory &);

public:
    NodePacketHistory(Packet *, NodePacketHistory *);

    std::list<Packet *> get_packets() const;
    std::list<Packet *> get_packets_since(NodePacketHistory *start) const;
    bool contains(NodePacketHistory *) const;
};

bool operator==(const NodePacketHistory &, const NodePacketHistory &);

/**
 * PacketHistory contains the packet traversal history (of each node) for the
 * whole network of the current EC.
 */
class PacketHistory {
private:
    std::unordered_map<Node *, NodePacketHistory *> tbl;

    friend struct PacketHistoryHash;
    friend bool operator==(const PacketHistory &, const PacketHistory &);

public:
    PacketHistory(const Network &);
    PacketHistory(const PacketHistory &) = default;
    PacketHistory(PacketHistory &&)      = default;

    void set_node_pkt_hist(Node *, NodePacketHistory *);
    NodePacketHistory *get_node_pkt_hist(Node *);
};

bool operator==(const PacketHistory &, const PacketHistory &);

struct NodePacketHistoryHash {
    size_t operator()(NodePacketHistory *const &) const;
};

struct NodePacketHistoryEq {
    bool operator()(NodePacketHistory *const &,
                    NodePacketHistory *const &) const;
};

struct NodePacketHistoryComp {
    bool operator()(NodePacketHistory *const &,
                    NodePacketHistory *const &) const;
};

struct PacketHistoryHash {
    size_t operator()(PacketHistory *const &) const;
};

struct PacketHistoryEq {
    bool operator()(PacketHistory *const &, PacketHistory *const &) const;
};
