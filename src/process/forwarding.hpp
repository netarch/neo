#pragma once

#include <vector>
#include <unordered_set>
#include <optional>

#include "process/process.hpp"
#include "policy/policy.hpp"
#include "fib.hpp"
#include "choices.hpp"
#include "packet.hpp"
#include "pkt-hist.hpp"
#include "network.hpp"
#include "middlebox.hpp"
class State;

enum fwd_mode {
    // start from 1 to avoid execution before configuration
    PACKET_ENTRY = 1,
    FIRST_COLLECT = 2,
    FIRST_FORWARD = 3,
    COLLECT_NHOPS = 4,
    FORWARD_PACKET = 5,
    ACCEPTED = 6,
    DROPPED = 7
};

struct CandHash {
    size_t operator()(const std::vector<FIB_IPNH> *const&) const;
};

struct CandEq {
    bool operator()(const std::vector<FIB_IPNH> *const&,
                    const std::vector<FIB_IPNH> *const&) const;
};

class ForwardingProcess : public Process
{
private:
    Policy *policy;

    std::unordered_set<std::vector<FIB_IPNH> *, CandHash, CandEq>
    candidates_hist;
    std::unordered_set<Choices *> choices_hist;

    // all history packets
    std::unordered_set<Packet *, PacketHash, PacketEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *, NodePacketHistoryHash,
        NodePacketHistoryEq> node_pkt_hist_hist;
    // history of packet histories
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq>
    pkt_hist_hist;

    void packet_entry(State *) const;
    void first_collect(State *, Network&);
    void first_forward(State *);
    void collect_next_hops(State *, Network&);
    void forward_packet(State *);
    void accepted(State *, Network&);
    void dropped(State *) const;

    void phase_transition(State *, Network&, uint8_t next_pkt_state,
                          bool change_direction);

    std::set<FIB_IPNH> inject_packet(State *, Middlebox *, Network&);
    void update_candidates(State *, const std::vector<FIB_IPNH>&);
    void update_choices(State *, Choices&&);
    void add_choice(State *, const FIB_IPNH&);
    std::optional<FIB_IPNH> get_choice(State *);

public:
    ForwardingProcess() = default;
    ForwardingProcess(const ForwardingProcess&) = delete;
    ForwardingProcess(ForwardingProcess&&) = delete;
    ~ForwardingProcess();

    ForwardingProcess& operator=(const ForwardingProcess&) = delete;
    ForwardingProcess& operator=(ForwardingProcess&&) = delete;

    void init(State *, Network&, Policy *);
    void exec_step(State *, Network&) override;
};
