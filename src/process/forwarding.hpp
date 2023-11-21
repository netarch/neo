#pragma once

#include <list>
#include <map>
#include <unordered_set>

#include "injection-result.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
#include "process/process.hpp"
class Network;
class FIB_IPNH;
class Middlebox;

enum fwd_mode {
    FIRST_COLLECT,  // -> FIRST_FORWARD
    FIRST_FORWARD,  // -> COLLECT_NHOPS, ACCEPTED
    COLLECT_NHOPS,  // -> FORWARD_PACKET, CHOOSE_INJ_RES, DROPPED
    CHOOSE_INJ_RES, // -> FIRST_FORWARD, FORWARD_PACKET, DROPPED
    FORWARD_PACKET, // -> COLLECT_NHOPS, ACCEPTED
    ACCEPTED,       // -> FIRST_COLLECT, (end)
    DROPPED         // -> (end)
};

class ForwardingProcess : public Process {
private:
    // TODO: Move to UniqueStorage.
    // all history packets
    std::unordered_set<Packet *, PacketPtrHash, PacketPtrEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *,
                       NodePacketHistoryHash,
                       NodePacketHistoryEq>
        node_pkt_hist_hist;

    void first_collect();
    void first_forward();
    void collect_next_hops();
    void choose_injection_results();
    void forward_packet();
    void accepted();

    void phase_transition(uint8_t next_proto_state, bool opposite_dir) const;
    void inject_packet(Middlebox *);
    void update_model_from_injection_result(Middlebox *,
                                            const InjectionResult &) const;

public:
    ~ForwardingProcess();

    void init(const Network &);
    void exec_step() override;
    void reset() override;
};
