#pragma once

#include <vector>
#include <unordered_set>

#include "process/process.hpp"
class Policy;
class Policies;
#include "policy/policy.hpp"
#include "node.hpp"
#include "pkt-hist.hpp"
#include "middlebox.hpp"

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

    // all history packets
    std::unordered_set<Packet *, PacketHash, PacketEq> all_pkts;
    // history of node packet histories
    std::unordered_set<NodePacketHistory *, NodePacketHistoryHash,
        NodePacketHistoryEq> node_pkt_hist_hist;
    // history of packet histories
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq>
    pkt_hist_hist;

    void packet_entry(State *) const;
    void first_collect(State *);
    void first_forward(State *) const;
    void collect_next_hops(State *);
    void forward_packet(State *) const;
    void accepted(State *, Network&);

    std::set<FIB_IPNH> inject_packet(State *, Middlebox *,
                                     const IPv4Address& dst_ip);
    void update_candidates(State *, const std::vector<FIB_IPNH>&);

public:
    ForwardingProcess() = default;
    ForwardingProcess(const ForwardingProcess&) = delete;
    ForwardingProcess(ForwardingProcess&&) = delete;
    ~ForwardingProcess();

    ForwardingProcess& operator=(const ForwardingProcess&) = delete;
    ForwardingProcess& operator=(ForwardingProcess&&) = delete;

    void init(State *, const Network&, Policy *);
    void exec_step(State *, Network&) override;
};
