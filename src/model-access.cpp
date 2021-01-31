#include "model-access.hpp"

#include <cstring>
#include <utility>
#include <unordered_set>

#include "fib.hpp"
#include "pkt-hist.hpp"
#include "choices.hpp"
#include "process/openflow.hpp"
#include "candidates.hpp"
#include "model.h"


/* variables for preventing duplicates */

class VariableHist
{
public:
    std::unordered_set<FIB *, FIBHash, FIBEq> fib_hist;
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq> pkt_hist_hist;
    std::unordered_set<Choices *, ChoicesHash, ChoicesEq> path_choices_hist;
    std::unordered_set<OpenflowUpdateState *,
        OFUpdateStateHash, OFUpdateStateEq> openflow_update_state_hist;
    std::unordered_set<Candidates *, CandHash, CandEq> candidates_hist;

    VariableHist() = default;
    ~VariableHist();
};

VariableHist::~VariableHist()
{
    for (FIB *fib : this->fib_hist) {
        delete fib;
    }
    for (PacketHistory *pkt_hist : this->pkt_hist_hist) {
        delete pkt_hist;
    }
    for (Choices *path_choices : this->path_choices_hist) {
        delete path_choices;
    }
    for (OpenflowUpdateState *update_state : this->openflow_update_state_hist) {
        delete update_state;
    }
    for (Candidates *candidates : candidates_hist) {
        delete candidates;
    }
}

static VariableHist storage;


FIB *get_fib(State *state)
{
    FIB *fib;
    memcpy(&fib, state->comm_state[state->comm].fib, sizeof(FIB *));
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
    memcpy(state->comm_state[state->comm].fib, &new_fib, sizeof(FIB *));
    return new_fib;
}

int get_process_id(State *state)
{
    return state->comm_state[state->comm].process_id;
}

int set_process_id(State *state, int process_id)
{
    return state->comm_state[state->comm].process_id = process_id;
}

int get_pkt_state(State *state)
{
    return state->comm_state[state->comm].pkt_state;
}

int set_pkt_state(State *state, int pkt_state)
{
    return state->comm_state[state->comm].pkt_state = pkt_state;
}

int get_fwd_mode(State *state)
{
    return state->comm_state[state->comm].fwd_mode;
}

int set_fwd_mode(State *state, int fwd_mode)
{
    return state->comm_state[state->comm].fwd_mode = fwd_mode;
}

EqClass *get_ec(State *state)
{
    EqClass *ec;
    memcpy(&ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
    return ec;
}

EqClass *set_ec(State *state, EqClass *ec)
{
    memcpy(state->comm_state[state->comm].ec, &ec, sizeof(EqClass *));
    return ec;
}

uint32_t get_src_ip(State *state)
{
    uint32_t src_ip;
    memcpy(&src_ip, state->comm_state[state->comm].src_ip, sizeof(uint32_t));
    return src_ip;
}

uint32_t set_src_ip(State *state, uint32_t src_ip)
{
    memcpy(state->comm_state[state->comm].src_ip, &src_ip, sizeof(uint32_t));
    return src_ip;
}

uint32_t get_seq(State *state)
{
    uint32_t seq;
    memcpy(&seq, state->comm_state[state->comm].seq, sizeof(uint32_t));
    return seq;
}

uint32_t set_seq(State *state, uint32_t seq)
{
    memcpy(state->comm_state[state->comm].seq, &seq, sizeof(uint32_t));
    return seq;
}

uint32_t get_ack(State *state)
{
    uint32_t ack;
    memcpy(&ack, state->comm_state[state->comm].ack, sizeof(uint32_t));
    return ack;
}

uint32_t set_ack(State *state, uint32_t ack)
{
    memcpy(state->comm_state[state->comm].ack, &ack, sizeof(uint32_t));
    return ack;
}

uint16_t get_src_port(State *state)
{
    return state->comm_state[state->comm].src_port;
}

uint16_t set_src_port(State *state, uint16_t src_port)
{
    return state->comm_state[state->comm].src_port = src_port;
}

uint16_t get_dst_port(State *state)
{
    return state->comm_state[state->comm].dst_port;
}

uint16_t set_dst_port(State *state, uint16_t dst_port)
{
    return state->comm_state[state->comm].dst_port = dst_port;
}

Node *get_src_node(State *state)
{
    Node *src_node;
    memcpy(&src_node, state->comm_state[state->comm].src_node, sizeof(Node *));
    return src_node;
}

Node *set_src_node(State *state, Node *src_node)
{
    memcpy(state->comm_state[state->comm].src_node, &src_node, sizeof(Node *));
    return src_node;
}

Node *get_tx_node(State *state)
{
    Node *tx_node;
    memcpy(&tx_node, state->comm_state[state->comm].tx_node, sizeof(Node *));
    return tx_node;
}

Node *set_tx_node(State *state, Node *tx_node)
{
    memcpy(state->comm_state[state->comm].tx_node, &tx_node, sizeof(Node *));
    return tx_node;
}

Node *get_rx_node(State *state)
{
    Node *rx_node;
    memcpy(&rx_node, state->comm_state[state->comm].rx_node, sizeof(Node *));
    return rx_node;
}

Node *set_rx_node(State *state, Node *rx_node)
{
    memcpy(state->comm_state[state->comm].rx_node, &rx_node, sizeof(Node *));
    return rx_node;
}

PacketHistory *get_pkt_hist(State *state)
{
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->comm_state[state->comm].pkt_hist, sizeof(PacketHistory *));
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
    memcpy(state->comm_state[state->comm].pkt_hist, &new_pkt_hist, sizeof(PacketHistory *));
    return new_pkt_hist;
}

Node *get_pkt_location(State *state)
{
    Node *pkt_location;
    memcpy(&pkt_location, state->comm_state[state->comm].pkt_location, sizeof(Node *));
    return pkt_location;
}

Node *set_pkt_location(State *state, Node *pkt_location)
{
    memcpy(state->comm_state[state->comm].pkt_location, &pkt_location, sizeof(Node *));
    return pkt_location;
}

Interface *get_ingress_intf(State *state)
{
    Interface *ingress_intf;
    memcpy(&ingress_intf, state->comm_state[state->comm].ingress_intf, sizeof(Interface *));
    return ingress_intf;
}

Interface *set_ingress_intf(State *state, Interface *ingress_intf)
{
    memcpy(state->comm_state[state->comm].ingress_intf, &ingress_intf, sizeof(Interface *));
    return ingress_intf;
}

Choices *get_path_choices(State *state)
{
    Choices *path_choices;
    memcpy(&path_choices, state->comm_state[state->comm].path_choices, sizeof(Choices *));
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
    memcpy(state->comm_state[state->comm].path_choices, &new_path_choices, sizeof(Choices *));
    return new_path_choices;
}

OpenflowUpdateState *get_openflow_update_state(State *state)
{
    OpenflowUpdateState *update_state;
    memcpy(&update_state, state->comm_state[state->comm].openflow_update_state,
           sizeof(OpenflowUpdateState *));
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
    memcpy(state->comm_state[state->comm].openflow_update_state,
           &new_state, sizeof(OpenflowUpdateState *));
    return new_state;
}

int get_repetition(State *state)
{
    return state->comm_state[state->comm].repetition;
}

int set_repetition(State *state, int repetition)
{
    return state->comm_state[state->comm].repetition = repetition;
}

bool get_violated(State *state)
{
    return state->violated;
}

bool set_violated(State *state, bool violated)
{
    return (state->violated = violated);
}

int get_comm(State *state)
{
    return state->comm;
}

int set_comm(State *state, int comm)
{
    return state->comm = comm;
}

int get_num_comms(State *state)
{
    return state->num_comms;
}

int set_num_comms(State *state, int num_comms)
{
    return state->num_comms = num_comms;
}

int get_correlated_policy_idx(State *state)
{
    return state->correlated_policy_idx;
}

int set_correlated_policy_idx(State *state, int correlated_policy_idx)
{
    return state->correlated_policy_idx = correlated_policy_idx;
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

Candidates *get_candidates(State *state)
{
    Candidates *candidates;
    memcpy(&candidates, state->candidates, sizeof(Candidates *));
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
    memcpy(state->candidates, &new_candidates, sizeof(Candidates *));
    state->choice_count = new_candidates->size();
    return new_candidates;
}
