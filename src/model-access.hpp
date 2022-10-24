#pragma once

#include <cstdint>

class EqClass;
class Node;
class Interface;
class Candidates;
class FIB;
class Choices;
class PacketHistory;
class OpenflowUpdateState;
class ReachCounts;
struct State;

/* per-connection state variables */

void print_conn_states(State *);

// control state
int get_executable(State *);
int set_executable(State *, int);
// flow information
uint16_t get_proto_state(State *);
uint16_t set_proto_state(State *, int);
uint32_t get_src_ip(State *);
uint32_t set_src_ip(State *, uint32_t);
EqClass *get_dst_ip_ec(State *);
EqClass *set_dst_ip_ec(State *, EqClass *);
uint16_t get_src_port(State *);
uint16_t set_src_port(State *, uint16_t);
uint16_t get_dst_port(State *);
uint16_t set_dst_port(State *, uint16_t);
uint32_t get_seq(State *);
uint32_t set_seq(State *, uint32_t);
uint32_t get_ack(State *);
uint32_t set_ack(State *, uint32_t);
Node *get_src_node(State *);
Node *set_src_node(State *, Node *);
Node *get_tx_node(State *);
Node *set_tx_node(State *, Node *);
Node *get_rx_node(State *);
Node *set_rx_node(State *, Node *);
// forwarding information
int get_fwd_mode(State *);
int set_fwd_mode(State *, int);
Node *get_pkt_location(State *);
Node *set_pkt_location(State *, Node *);
Interface *get_ingress_intf(State *);
Interface *set_ingress_intf(State *, Interface *);
Candidates *get_candidates(State *);
Candidates *set_candidates(State *, Candidates &&);
Candidates *reset_candidates(State *);
// per-flow data plane state
FIB *get_fib(State *);
FIB *set_fib(State *, FIB &&);
Choices *get_path_choices(State *);
Choices *set_path_choices(State *, Choices &&);

/* non-connection specific, system-wide state variables */

// control state
int get_process_id(State *);
int set_process_id(State *, int);
int get_choice(State *);
int set_choice(State *, int);
int get_choice_count(State *);
int set_choice_count(State *, int);
int get_conn(State *);
int set_conn(State *, int);
int get_num_conns(State *);
int set_num_conns(State *, int);
// data plane state
PacketHistory *get_pkt_hist(State *);
PacketHistory *set_pkt_hist(State *, PacketHistory &&);
OpenflowUpdateState *get_openflow_update_state(State *);
OpenflowUpdateState *set_openflow_update_state(State *, OpenflowUpdateState &&);
// policy
bool get_violated(State *);
bool set_violated(State *, bool);
int get_correlated_policy_idx(State *);
int set_correlated_policy_idx(State *, int);
// loadbalance policy
ReachCounts *get_reach_counts(State *);
ReachCounts *set_reach_counts(State *, ReachCounts &&);
