#pragma once

#include <unordered_set>

#include "process/process.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
class Network;
class FIB_IPNH;
class Middlebox;
struct State;

/* TODO (check execution logic):
 * fwd 1st step: collect next hops and choose the collected next hops with spin
 * by updating candidates (then continue to the 2nd step without looking at
 * other processes)
 * fwd 2nd step: forward packet and update the ingress interface (allow
 * non-deterministic execution of other processes after this step)
 */

enum fwd_mode {
    FIRST_COLLECT,  // -> FIRST_FORWARD
    FIRST_FORWARD,  // -> COLLECT_NHOPS, ACCEPTED
    COLLECT_NHOPS,  // -> FORWARD_PACKET, DROPPED
    FORWARD_PACKET, // -> COLLECT_NHOPS, ACCEPTED
    ACCEPTED,       // -> FIRST_COLLECT, (end)
    DROPPED         // -> (end)
};

class ForwardingProcess : public Process
{
private:
    // all history packets
    std::unordered_set<Packet *, PacketHash, PacketEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *, NodePacketHistoryHash,
        NodePacketHistoryEq> node_pkt_hist_hist;

    void first_collect(State *);
    void first_forward(State *);
    void collect_next_hops(State *);
    void forward_packet(State *);
    void accepted(State *, const Network&);

    void phase_transition(State *, const Network&, uint8_t next_proto_state,
                          bool change_direction);
    void identify_conn(State *, const Packet&, int& conn,
                       bool& change_direction) const;
    std::set<FIB_IPNH> inject_packet(State *, Middlebox *);

public:
    ForwardingProcess() = default;
    ~ForwardingProcess();

    void init(State *, const Network&);
    void exec_step(State *, const Network&) override;
};
