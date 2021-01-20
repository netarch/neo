#pragma once

#include <string>
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

std::string fwd_mode_to_str(int);

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

    std::unordered_set<std::vector<FIB_IPNH> *, CandHash, CandEq> candidates_hist;
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
    bool same_dir_comm(State *, int comm, const Packet&) const;
    bool opposite_dir_comm(State *, int comm, const Packet&) const;
    int find_comm(State *, const Packet&) const;
    void convert_tcp_flags(State *, int comm, Packet&) const;

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

    // initialize the forwarding process when system starts
    void init(State *, Network&, Policy *);
    // reset the forwarding process without changing pkt_hist and path_choices
    void reset(State *, Network&, Policy *);
    void exec_step(State *, Network&) override;
};
