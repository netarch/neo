#pragma once

#include <cstdint>

class FIB;
class EqClass;
class Node;
class PacketHistory;
class Interface;
class Choices;
class OpenflowUpdateState;
class Candidates;
struct State;

/* per-communication state variables */

// network
FIB *get_fib(State *);
FIB *set_fib(State *, FIB&&);
// plankton process
int get_process_id(State *);
int set_process_id(State *, int);
// forwarding process
int get_pkt_state(State *);
int set_pkt_state(State *, int);
int get_fwd_mode(State *);
int set_fwd_mode(State *, int);
EqClass *get_ec(State *);
EqClass *set_ec(State *, EqClass *);
uint32_t get_src_ip(State *);
uint32_t set_src_ip(State *, uint32_t);
uint32_t get_seq(State *);
uint32_t set_seq(State *, uint32_t);
uint32_t get_ack(State *);
uint32_t set_ack(State *, uint32_t);
uint16_t get_src_port(State *);
uint16_t set_src_port(State *, uint16_t);
uint16_t get_dst_port(State *);
uint16_t set_dst_port(State *, uint16_t);
Node *get_src_node(State *);
Node *set_src_node(State *, Node *);
Node *get_tx_node(State *);
Node *set_tx_node(State *, Node *);
Node *get_rx_node(State *);
Node *set_rx_node(State *, Node *);
PacketHistory *get_pkt_hist(State *);
PacketHistory *set_pkt_hist(State *, PacketHistory&&);
Node *get_pkt_location(State *);
Node *set_pkt_location(State *, Node *);
Interface *get_ingress_intf(State *);
Interface *set_ingress_intf(State *, Interface *);
Choices *get_path_choices(State *);
Choices *set_path_choices(State *, Choices&&);
// openflow process
OpenflowUpdateState *get_openflow_update_state(State *);
OpenflowUpdateState *set_openflow_update_state(State *, OpenflowUpdateState&&);
// load balance policy
int get_repetition(State *);
int set_repetition(State *, int);

/* general state variables */

// policy
bool get_violated(State *);
bool set_violated(State *, bool);
int get_comm(State *);
int set_comm(State *, int);
int get_num_comms(State *);
int set_num_comms(State *, int);
int get_correlated_policy_idx(State *);
int set_correlated_policy_idx(State *, int);

// general
int get_choice(State *);
int set_choice(State *, int);
int get_choice_count(State *);
int set_choice_count(State *, int);
Candidates *get_candidates(State *);
Candidates *set_candidates(State *, Candidates&&);
