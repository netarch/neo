#pragma once

#include <list>
#include <map>
#include <unordered_set>

#include "packet.hpp"
#include "pkt-hist.hpp"
#include "process/process.hpp"
class Network;
class FIB_IPNH;
class Middlebox;
struct State;

enum fwd_mode {
    FIRST_COLLECT,  // -> FIRST_FORWARD
    FIRST_FORWARD,  // -> COLLECT_NHOPS, ACCEPTED
    COLLECT_NHOPS,  // -> FORWARD_PACKET, DROPPED
    FORWARD_PACKET, // -> COLLECT_NHOPS, ACCEPTED
    ACCEPTED,       // -> FIRST_COLLECT, (end)
    DROPPED         // -> (end)
};

class ForwardingProcess : public Process {
private:
    // all history packets
    std::unordered_set<Packet *, PacketHash, PacketEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *,
                       NodePacketHistoryHash,
                       NodePacketHistoryEq>
        node_pkt_hist_hist;

    void first_collect(State *, const Network &);
    void first_forward(State *);
    void collect_next_hops(State *, const Network &);
    void forward_packet(State *);
    void accepted(State *, const Network &);

    void phase_transition(State *,
                          const Network &,
                          uint8_t next_proto_state,
                          bool opposite_dir) const;
    void inject_packet(State *, Middlebox *, const Network &);
    void process_recv_pkts(State *,
                           Middlebox *,
                           std::list<Packet> &&,
                           const Network &) const;
    void
    identify_conn(State *, Packet &, bool &is_new, bool &opposite_dir) const;
    void check_proto_state(const Packet &recv_pkt,
                           bool is_new,
                           uint8_t old_proto_state,
                           bool &next_phase) const;
    void check_seq_ack(State *,
                       const Packet &,
                       bool is_new,
                       bool opposite_dir,
                       bool next_phase) const;

public:
    ForwardingProcess() = default;
    ~ForwardingProcess();

    void init(State *, const Network &);
    void exec_step(State *, const Network &) override;
};
