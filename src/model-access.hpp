#pragma once

#include <cstdint>

class Candidates;
class Choices;
class EqClass;
class FIB;
class InjectionResults;
class Interface;
class Network;
class Node;
class OpenflowProcess;
class OpenflowUpdateState;
class PacketHistory;
class Payload;
class ReachCounts;
class VisitedHops;
struct State;

class Model {
private:
    // Since `now` is a global variable defined in `model.c`, we should be able
    // to safely assume that the pointer to that variable remains the same
    // through out the entire execution of the spin program.
    State *state;
    Network *network;
    OpenflowProcess *openflow;

    Model();
    friend class API;
    friend class Plankton;
    void set_state(State *);
    void init(Network *, OpenflowProcess *);
    void reset();

public:
    // Disable the copy constructor and the copy assignment operator
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    static Model &get();

    /**
     * per-connection state variables
     */

    void print_conn_states() const;

    // control state
    int get_executable() const;
    int get_executable_for_conn(int) const;
    int set_executable(int) const;
    // flow information
    uint16_t get_proto_state() const;
    uint16_t get_proto_state_for_conn(int) const;
    uint16_t set_proto_state(int) const;
    uint32_t get_src_ip() const;
    uint32_t set_src_ip(uint32_t) const;
    EqClass *get_dst_ip_ec() const;
    EqClass *set_dst_ip_ec(EqClass *) const;
    uint16_t get_src_port() const;
    uint16_t set_src_port(uint16_t) const;
    uint16_t get_dst_port() const;
    uint16_t set_dst_port(uint16_t) const;
    uint32_t get_seq() const;
    uint32_t set_seq(uint32_t) const;
    uint32_t get_ack() const;
    uint32_t set_ack(uint32_t) const;
    Payload *get_payload() const;
    Payload *set_payload(Payload *) const;
    Node *get_src_node() const;
    Node *set_src_node(Node *) const;
    Node *get_tx_node() const;
    Node *set_tx_node(Node *) const;
    Node *get_rx_node() const;
    Node *set_rx_node(Node *) const;
    // forwarding information
    int get_fwd_mode() const;
    int get_fwd_mode_for_conn(int) const;
    int set_fwd_mode(int) const;
    Node *get_pkt_location() const;
    Node *set_pkt_location(Node *) const;
    Interface *get_ingress_intf() const;
    Interface *set_ingress_intf(Interface *) const;
    Candidates *get_candidates() const;
    Candidates *set_candidates(Candidates &&) const;
    Candidates *reset_candidates() const;
    InjectionResults *get_injection_results() const;
    InjectionResults *set_injection_results(InjectionResults &&) const;
    InjectionResults *set_injection_results(InjectionResults *) const;
    InjectionResults *reset_injection_results() const;
    // per-flow data plane state
    FIB *get_fib() const;
    FIB *set_fib(FIB &&) const;
    void update_fib() const; // update FIB according to the current EC
    Choices *get_path_choices() const;
    Choices *set_path_choices(Choices &&) const;
    // loop invariant
    VisitedHops *get_visited_hops() const;
    VisitedHops *set_visited_hops(VisitedHops &&) const;

    /**
     * non-connection specific, system-wide state variables
     */

    // control state
    int get_process_id() const;
    int set_process_id(int) const;
    int get_choice() const;
    int set_choice(int) const;
    int get_choice_count() const;
    int set_choice_count(int) const;
    int get_conn() const;
    int set_conn(int) const;
    int get_num_conns() const;
    int set_num_conns(int) const;
    // data plane state
    PacketHistory *get_pkt_hist() const;
    PacketHistory *set_pkt_hist(PacketHistory &&) const;
    OpenflowUpdateState *get_openflow_update_state() const;
    OpenflowUpdateState *
    set_openflow_update_state(OpenflowUpdateState &&) const;
    // invariant
    bool get_violated() const;
    bool set_violated(bool) const;
    int get_correlated_inv_idx() const;
    int set_correlated_inv_idx(int) const;
    // loadbalance invariant
    ReachCounts *get_reach_counts() const;
    ReachCounts *set_reach_counts(ReachCounts &&) const;
};

extern Model &model;
