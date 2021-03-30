#include "model-access.hpp"

#include <cstring>
#include <utility>
#include <unordered_set>

#include "eqclass.hpp"
#include "node.hpp"
#include "interface.hpp"
#include "candidates.hpp"
#include "fib.hpp"
#include "choices.hpp"
#include "pkt-hist.hpp"
#include "process/openflow.hpp"
#include "policy/loadbalance.hpp"
#include "model.h"


/* variables for preventing duplicates */

class VariableHist
{
public:
    std::unordered_set<Candidates *, CandHash, CandEq> candidates_hist;
    std::unordered_set<FIB *, FIBHash, FIBEq> fib_hist;
    std::unordered_set<Choices *, ChoicesHash, ChoicesEq> path_choices_hist;
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq> pkt_hist_hist;
    std::unordered_set<OpenflowUpdateState *,
        OFUpdateStateHash, OFUpdateStateEq> openflow_update_state_hist;
    std::unordered_set<ReachCounts *, ReachCountsHash, ReachCountsEq> reach_counts_hist;

    VariableHist() = default;
    ~VariableHist();
};

VariableHist::~VariableHist()
{
    for (Candidates *candidates : candidates_hist) {
        delete candidates;
    }
    for (FIB *fib : this->fib_hist) {
        delete fib;
    }
    for (Choices *path_choices : this->path_choices_hist) {
        delete path_choices;
    }
    for (PacketHistory *pkt_hist : this->pkt_hist_hist) {
        delete pkt_hist;
    }
    for (OpenflowUpdateState *update_state : this->openflow_update_state_hist) {
        delete update_state;
    }
    for (ReachCounts *reach_counts : this->reach_counts_hist) {
        delete reach_counts;
    }
}

static VariableHist storage;


bool get_is_executable(State *state)
{
    return state->conn_state[state->conn].is_executable;
}

bool set_is_executable(State *state, bool is_executable)
{
    return (state->conn_state[state->conn].is_executable = is_executable);
}

int get_proto_state(State *state)
{
    return state->conn_state[state->conn].proto_state;
}

int set_proto_state(State *state, int proto_state)
{
    return state->conn_state[state->conn].proto_state = proto_state;
}

uint32_t get_src_ip(State *state)
{
    uint32_t src_ip;
    memcpy(&src_ip, state->conn_state[state->conn].src_ip, sizeof(uint32_t));
    return src_ip;
}

uint32_t set_src_ip(State *state, uint32_t src_ip)
{
    memcpy(state->conn_state[state->conn].src_ip, &src_ip, sizeof(uint32_t));
    return src_ip;
}

EqClass *get_dst_ip_ec(State *state)
{
    EqClass *dst_ip_ec;
    memcpy(&dst_ip_ec, state->conn_state[state->conn].dst_ip_ec, sizeof(EqClass *));
    return dst_ip_ec;
}

EqClass *set_dst_ip_ec(State *state, EqClass *dst_ip_ec)
{
    memcpy(state->conn_state[state->conn].dst_ip_ec, &dst_ip_ec, sizeof(EqClass *));
    return dst_ip_ec;
}

uint16_t get_src_port(State *state)
{
    return state->conn_state[state->conn].src_port;
}

uint16_t set_src_port(State *state, uint16_t src_port)
{
    return state->conn_state[state->conn].src_port = src_port;
}

uint16_t get_dst_port(State *state)
{
    return state->conn_state[state->conn].dst_port;
}

uint16_t set_dst_port(State *state, uint16_t dst_port)
{
    return state->conn_state[state->conn].dst_port = dst_port;
}

uint32_t get_seq(State *state)
{
    uint32_t seq;
    memcpy(&seq, state->conn_state[state->conn].seq, sizeof(uint32_t));
    return seq;
}

uint32_t set_seq(State *state, uint32_t seq)
{
    memcpy(state->conn_state[state->conn].seq, &seq, sizeof(uint32_t));
    return seq;
}

uint32_t get_ack(State *state)
{
    uint32_t ack;
    memcpy(&ack, state->conn_state[state->conn].ack, sizeof(uint32_t));
    return ack;
}

uint32_t set_ack(State *state, uint32_t ack)
{
    memcpy(state->conn_state[state->conn].ack, &ack, sizeof(uint32_t));
    return ack;
}

Node *get_src_node(State *state)
{
    Node *src_node;
    memcpy(&src_node, state->conn_state[state->conn].src_node, sizeof(Node *));
    return src_node;
}

Node *set_src_node(State *state, Node *src_node)
{
    memcpy(state->conn_state[state->conn].src_node, &src_node, sizeof(Node *));
    return src_node;
}

Node *get_tx_node(State *state)
{
    Node *tx_node;
    memcpy(&tx_node, state->conn_state[state->conn].tx_node, sizeof(Node *));
    return tx_node;
}

Node *set_tx_node(State *state, Node *tx_node)
{
    memcpy(state->conn_state[state->conn].tx_node, &tx_node, sizeof(Node *));
    return tx_node;
}

Node *get_rx_node(State *state)
{
    Node *rx_node;
    memcpy(&rx_node, state->conn_state[state->conn].rx_node, sizeof(Node *));
    return rx_node;
}

Node *set_rx_node(State *state, Node *rx_node)
{
    memcpy(state->conn_state[state->conn].rx_node, &rx_node, sizeof(Node *));
    return rx_node;
}

int get_fwd_mode(State *state)
{
    return state->conn_state[state->conn].fwd_mode;
}

int set_fwd_mode(State *state, int fwd_mode)
{
    return state->conn_state[state->conn].fwd_mode = fwd_mode;
}

Node *get_pkt_location(State *state)
{
    Node *pkt_location;
    memcpy(&pkt_location, state->conn_state[state->conn].pkt_location, sizeof(Node *));
    return pkt_location;
}

Node *set_pkt_location(State *state, Node *pkt_location)
{
    memcpy(state->conn_state[state->conn].pkt_location, &pkt_location, sizeof(Node *));
    return pkt_location;
}

Interface *get_ingress_intf(State *state)
{
    Interface *ingress_intf;
    memcpy(&ingress_intf, state->conn_state[state->conn].ingress_intf, sizeof(Interface *));
    return ingress_intf;
}

Interface *set_ingress_intf(State *state, Interface *ingress_intf)
{
    memcpy(state->conn_state[state->conn].ingress_intf, &ingress_intf, sizeof(Interface *));
    return ingress_intf;
}

Candidates *get_candidates(State *state)
{
    Candidates *candidates;
    memcpy(&candidates, state->conn_state[state->conn].candidates, sizeof(Candidates *));
    return candidates;
}

Candidates *set_candidates(State *state, Candidates&& candidates)
{
    Candidates *new_candidates = new Candidates(std::move(candidates));
    auto res = storage.candidates_hist.insert(new_candidates);
    if (!res.second) {
        delete new_candidates;
        new_candidates = *(res.first);
    }
    memcpy(state->conn_state[state->conn].candidates, &new_candidates, sizeof(Candidates *));
    state->choice_count = new_candidates->size();
    return new_candidates;
}

Candidates *reset_candidates(State *state)
{
    memset(state->conn_state[state->conn].candidates, 0, sizeof(Candidates *));
    return nullptr;
}

FIB *get_fib(State *state)
{
    FIB *fib;
    memcpy(&fib, state->conn_state[state->conn].fib, sizeof(FIB *));
    return fib;
}

FIB *set_fib(State *state, FIB&& fib)
{
    FIB *new_fib = new FIB(std::move(fib));
    auto res = storage.fib_hist.insert(new_fib);
    if (!res.second) {
        delete new_fib;
        new_fib = *(res.first);
    }
    memcpy(state->conn_state[state->conn].fib, &new_fib, sizeof(FIB *));
    return new_fib;
}

Choices *get_path_choices(State *state)
{
    Choices *path_choices;
    memcpy(&path_choices, state->conn_state[state->conn].path_choices, sizeof(Choices *));
    return path_choices;
}

Choices *set_path_choices(State *state, Choices&& path_choices)
{
    Choices *new_path_choices = new Choices(std::move(path_choices));
    auto res = storage.path_choices_hist.insert(new_path_choices);
    if (!res.second) {
        delete new_path_choices;
        new_path_choices = *(res.first);
    }
    memcpy(state->conn_state[state->conn].path_choices, &new_path_choices, sizeof(Choices *));
    return new_path_choices;
}

int get_process_id(State *state)
{
    return state->process_id;
}

int set_process_id(State *state, int process_id)
{
    return state->process_id = process_id;
}

int get_choice(State *state)
{
    return state->choice;
}

int set_choice(State *state, int choice)
{
    return state->choice = choice;
}

int get_choice_count(State *state)
{
    return state->choice_count;
}

int set_choice_count(State *state, int choice_count)
{
    return state->choice_count = choice_count;
}

int get_conn(State *state)
{
    return state->conn;
}

int set_conn(State *state, int conn)
{
    return state->conn = conn;
}

int get_num_conns(State *state)
{
    return state->num_conns;
}

int set_num_conns(State *state, int num_conns)
{
    return state->num_conns = num_conns;
}

PacketHistory *get_pkt_hist(State *state)
{
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->pkt_hist, sizeof(PacketHistory *));
    return pkt_hist;
}

PacketHistory *set_pkt_hist(State *state, PacketHistory&& pkt_hist)
{
    PacketHistory *new_pkt_hist = new PacketHistory(std::move(pkt_hist));
    auto res = storage.pkt_hist_hist.insert(new_pkt_hist);
    if (!res.second) {
        delete new_pkt_hist;
        new_pkt_hist = *(res.first);
    }
    memcpy(state->pkt_hist, &new_pkt_hist, sizeof(PacketHistory *));
    return new_pkt_hist;
}

OpenflowUpdateState *get_openflow_update_state(State *state)
{
    OpenflowUpdateState *update_state;
    memcpy(&update_state, state->openflow_update_state, sizeof(OpenflowUpdateState *));
    return update_state;
}

OpenflowUpdateState *set_openflow_update_state(State *state, OpenflowUpdateState&& update_state)
{
    OpenflowUpdateState *new_state = new OpenflowUpdateState(std::move(update_state));
    auto res = storage.openflow_update_state_hist.insert(new_state);
    if (!res.second) {
        delete new_state;
        new_state = *(res.first);
    }
    memcpy(state->openflow_update_state, &new_state, sizeof(OpenflowUpdateState *));
    return new_state;
}

bool get_violated(State *state)
{
    return state->violated;
}

bool set_violated(State *state, bool violated)
{
    return (state->violated = violated);
}

int get_correlated_policy_idx(State *state)
{
    return state->correlated_policy_idx;
}

int set_correlated_policy_idx(State *state, int correlated_policy_idx)
{
    return state->correlated_policy_idx = correlated_policy_idx;
}

ReachCounts *get_reach_counts(State *state)
{
    ReachCounts *reach_counts;
    memcpy(&reach_counts, state->reach_counts, sizeof(ReachCounts *));
    return reach_counts;
}

ReachCounts *set_reach_counts(State *state, ReachCounts&& reach_counts)
{
    ReachCounts *new_reach_counts = new ReachCounts(std::move(reach_counts));
    auto res = storage.reach_counts_hist.insert(new_reach_counts);
    if (!res.second) {
        delete new_reach_counts;
        new_reach_counts = *(res.first);
    }
    memcpy(state->reach_counts, &new_reach_counts, sizeof(ReachCounts *));
    return new_reach_counts;
}
