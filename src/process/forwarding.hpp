#pragma once

#include <unordered_set>
#include <optional>

#include "process/process.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
class Policy;
class Network;
class FIB_IPNH;
class Middlebox;
struct State;

enum fwd_mode {
    PACKET_ENTRY,
    FIRST_COLLECT,
    FIRST_FORWARD,
    COLLECT_NHOPS,
    FORWARD_PACKET,
    ACCEPTED,
    DROPPED
};

class ForwardingProcess : public Process
{
private:
    Policy *policy;

    // all history packets
    std::unordered_set<Packet *, PacketHash, PacketEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *, NodePacketHistoryHash,
        NodePacketHistoryEq> node_pkt_hist_hist;

    void packet_entry(State *) const;
    void first_collect(State *);
    void first_forward(State *);
    void collect_next_hops(State *);
    void forward_packet(State *);
    void accepted(State *, Network&);
    void dropped(State *) const;

    void phase_transition(State *, Network&, uint8_t next_pkt_state,
                          bool change_direction);
    void identify_comm(State *, const Packet&, int& comm,
                       bool& change_direction) const;
    std::set<FIB_IPNH> inject_packet(State *, Middlebox *);

public:
    ForwardingProcess() = default;
    ~ForwardingProcess();

    // initialize the forwarding process when system starts
    void init(State *, Network&, Policy *);
    // reset the forwarding process without changing pkt_hist and path_choices
    void reset(State *, Network&);
    void exec_step(State *, Network&) override;
};
