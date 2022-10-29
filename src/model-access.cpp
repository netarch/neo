#include "model-access.hpp"

#include <cassert>
#include <cstring>
#include <unordered_set>
#include <utility>

#include "candidates.hpp"
#include "choices.hpp"
#include "eqclass.hpp"
#include "fib.hpp"
#include "interface.hpp"
#include "lib/logger.hpp"
#include "node.hpp"
#include "payload.hpp"
#include "pkt-hist.hpp"
#include "policy/loadbalance.hpp"
#include "process/openflow.hpp"
#include "protocols.hpp"
#include "reachcounts.hpp"

#include "model.h"

/* variables for preventing duplicates */

class VariableHist {
public:
    std::unordered_set<Candidates *, CandHash, CandEq> candidates_hist;
    std::unordered_set<FIB *, FIBHash, FIBEq> fib_hist;
    std::unordered_set<Choices *, ChoicesHash, ChoicesEq> path_choices_hist;
    std::unordered_set<PacketHistory *, PacketHistoryHash, PacketHistoryEq>
        pkt_hist_hist;
    std::
        unordered_set<OpenflowUpdateState *, OFUpdateStateHash, OFUpdateStateEq>
            openflow_update_state_hist;
    std::unordered_set<ReachCounts *, ReachCountsHash, ReachCountsEq>
        reach_counts_hist;

    VariableHist() = default;
    ~VariableHist();
};

VariableHist::~VariableHist() {
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
const Model &model = Model::get();

Model::Model() : state(nullptr) {}

Model &Model::get() {
    static Model instance;
    return instance;
}

void Model::_set_state(State *state) {
    if (this->state == nullptr) {
        this->state = state;
    } else {
        assert(this->state == state);
    }
}

void Model::print_conn_states() const {
    int orig_conn = get_conn();
    for (int conn = 0; conn < get_num_conns(); ++conn) {
        set_conn(conn);
        uint32_t src_ip = get_src_ip();
        std::string str = (conn == orig_conn ? "* " : "- ") +
                          std::to_string(conn) + ": [" +
                          std::string(PS_STR(get_proto_state())) + ":" +
                          std::to_string(get_proto_state()) + "] " +
                          get_src_node()->get_name() + "(" +
                          (src_ip == 0 ? std::string("null")
                                       : IPv4Address(src_ip).to_string()) +
                          "):" + std::to_string(get_src_port()) + " --> " +
                          get_dst_ip_ec()->to_string() + ":" +
                          std::to_string(get_dst_port()) +
                          " (loc: " + get_pkt_location()->get_name() +
                          ") executable: " + std::to_string(get_executable());
        Logger::info(str);
    }
    set_conn(orig_conn);
}

int Model::get_executable() const {
    return state->conn_state[state->conn].executable;
}

int Model::get_executable_for_conn(int conn) const {
    return state->conn_state[conn].executable;
}

int Model::set_executable(int executable) const {
    return (state->conn_state[state->conn].executable = executable);
}

uint16_t Model::get_proto_state() const {
    return state->conn_state[state->conn].proto_state;
}

uint16_t Model::get_proto_state_for_conn(int conn) const {
    return state->conn_state[conn].proto_state;
}

uint16_t Model::set_proto_state(int proto_state) const {
    return state->conn_state[state->conn].proto_state = proto_state;
}

uint32_t Model::get_src_ip() const {
    uint32_t src_ip;
    memcpy(&src_ip, state->conn_state[state->conn].src_ip, sizeof(uint32_t));
    return src_ip;
}

uint32_t Model::set_src_ip(uint32_t src_ip) const {
    memcpy(state->conn_state[state->conn].src_ip, &src_ip, sizeof(uint32_t));
    return src_ip;
}

EqClass *Model::get_dst_ip_ec() const {
    EqClass *dst_ip_ec;
    memcpy(&dst_ip_ec, state->conn_state[state->conn].dst_ip_ec,
           sizeof(EqClass *));
    return dst_ip_ec;
}

EqClass *Model::set_dst_ip_ec(EqClass *dst_ip_ec) const {
    memcpy(state->conn_state[state->conn].dst_ip_ec, &dst_ip_ec,
           sizeof(EqClass *));
    return dst_ip_ec;
}

uint16_t Model::get_src_port() const {
    return state->conn_state[state->conn].src_port;
}

uint16_t Model::set_src_port(uint16_t src_port) const {
    return state->conn_state[state->conn].src_port = src_port;
}

uint16_t Model::get_dst_port() const {
    return state->conn_state[state->conn].dst_port;
}

uint16_t Model::set_dst_port(uint16_t dst_port) const {
    return state->conn_state[state->conn].dst_port = dst_port;
}

uint32_t Model::get_seq() const {
    uint32_t seq;
    memcpy(&seq, state->conn_state[state->conn].seq, sizeof(uint32_t));
    return seq;
}

uint32_t Model::set_seq(uint32_t seq) const {
    memcpy(state->conn_state[state->conn].seq, &seq, sizeof(uint32_t));
    return seq;
}

uint32_t Model::get_ack() const {
    uint32_t ack;
    memcpy(&ack, state->conn_state[state->conn].ack, sizeof(uint32_t));
    return ack;
}

uint32_t Model::set_ack(uint32_t ack) const {
    memcpy(state->conn_state[state->conn].ack, &ack, sizeof(uint32_t));
    return ack;
}

Payload *Model::get_payload() const {
    Payload *payload;
    memcpy(&payload, state->conn_state[state->conn].payload, sizeof(Payload *));
    return payload;
}

Payload *Model::set_payload(Payload *payload) const {
    memcpy(state->conn_state[state->conn].payload, &payload, sizeof(Payload *));
    return payload;
}

Node *Model::get_src_node() const {
    Node *src_node;
    memcpy(&src_node, state->conn_state[state->conn].src_node, sizeof(Node *));
    return src_node;
}

Node *Model::set_src_node(Node *src_node) const {
    memcpy(state->conn_state[state->conn].src_node, &src_node, sizeof(Node *));
    return src_node;
}

Node *Model::get_tx_node() const {
    Node *tx_node;
    memcpy(&tx_node, state->conn_state[state->conn].tx_node, sizeof(Node *));
    return tx_node;
}

Node *Model::set_tx_node(Node *tx_node) const {
    memcpy(state->conn_state[state->conn].tx_node, &tx_node, sizeof(Node *));
    return tx_node;
}

Node *Model::get_rx_node() const {
    Node *rx_node;
    memcpy(&rx_node, state->conn_state[state->conn].rx_node, sizeof(Node *));
    return rx_node;
}

Node *Model::set_rx_node(Node *rx_node) const {
    memcpy(state->conn_state[state->conn].rx_node, &rx_node, sizeof(Node *));
    return rx_node;
}

int Model::get_fwd_mode() const {
    return state->conn_state[state->conn].fwd_mode;
}

int Model::get_fwd_mode_for_conn(int conn) const {
    return state->conn_state[conn].fwd_mode;
}

int Model::set_fwd_mode(int fwd_mode) const {
    return state->conn_state[state->conn].fwd_mode = fwd_mode;
}

Node *Model::get_pkt_location() const {
    Node *pkt_location;
    memcpy(&pkt_location, state->conn_state[state->conn].pkt_location,
           sizeof(Node *));
    return pkt_location;
}

Node *Model::set_pkt_location(Node *pkt_location) const {
    memcpy(state->conn_state[state->conn].pkt_location, &pkt_location,
           sizeof(Node *));
    return pkt_location;
}

Interface *Model::get_ingress_intf() const {
    Interface *ingress_intf;
    memcpy(&ingress_intf, state->conn_state[state->conn].ingress_intf,
           sizeof(Interface *));
    return ingress_intf;
}

Interface *Model::set_ingress_intf(Interface *ingress_intf) const {
    memcpy(state->conn_state[state->conn].ingress_intf, &ingress_intf,
           sizeof(Interface *));
    return ingress_intf;
}

Candidates *Model::get_candidates() const {
    Candidates *candidates;
    memcpy(&candidates, state->conn_state[state->conn].candidates,
           sizeof(Candidates *));
    return candidates;
}

Candidates *Model::set_candidates(Candidates &&candidates) const {
    Candidates *new_candidates = new Candidates(std::move(candidates));
    auto res = storage.candidates_hist.insert(new_candidates);
    if (!res.second) {
        delete new_candidates;
        new_candidates = *(res.first);
    }
    memcpy(state->conn_state[state->conn].candidates, &new_candidates,
           sizeof(Candidates *));
    state->choice_count = new_candidates->size();
    return new_candidates;
}

Candidates *Model::reset_candidates() const {
    memset(state->conn_state[state->conn].candidates, 0, sizeof(Candidates *));
    return nullptr;
}

FIB *Model::get_fib() const {
    FIB *fib;
    memcpy(&fib, state->conn_state[state->conn].fib, sizeof(FIB *));
    return fib;
}

FIB *Model::set_fib(FIB &&fib) const {
    FIB *new_fib = new FIB(std::move(fib));
    auto res = storage.fib_hist.insert(new_fib);
    if (!res.second) {
        delete new_fib;
        new_fib = *(res.first);
    }
    memcpy(state->conn_state[state->conn].fib, &new_fib, sizeof(FIB *));
    return new_fib;
}

Choices *Model::get_path_choices() const {
    Choices *path_choices;
    memcpy(&path_choices, state->conn_state[state->conn].path_choices,
           sizeof(Choices *));
    return path_choices;
}

Choices *Model::set_path_choices(Choices &&path_choices) const {
    Choices *new_path_choices = new Choices(std::move(path_choices));
    auto res = storage.path_choices_hist.insert(new_path_choices);
    if (!res.second) {
        delete new_path_choices;
        new_path_choices = *(res.first);
    }
    memcpy(state->conn_state[state->conn].path_choices, &new_path_choices,
           sizeof(Choices *));
    return new_path_choices;
}

int Model::get_process_id() const {
    return state->process_id;
}

int Model::set_process_id(int process_id) const {
    return state->process_id = process_id;
}

int Model::get_choice() const {
    return state->choice;
}

int Model::set_choice(int choice) const {
    return state->choice = choice;
}

int Model::get_choice_count() const {
    return state->choice_count;
}

int Model::set_choice_count(int choice_count) const {
    return state->choice_count = choice_count;
}

int Model::get_conn() const {
    return state->conn;
}

int Model::set_conn(int conn) const {
    return state->conn = conn;
}

int Model::get_num_conns() const {
    return state->num_conns;
}

int Model::set_num_conns(int num_conns) const {
    return state->num_conns = num_conns;
}

PacketHistory *Model::get_pkt_hist() const {
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->pkt_hist, sizeof(PacketHistory *));
    return pkt_hist;
}

PacketHistory *Model::set_pkt_hist(PacketHistory &&pkt_hist) const {
    PacketHistory *new_pkt_hist = new PacketHistory(std::move(pkt_hist));
    auto res = storage.pkt_hist_hist.insert(new_pkt_hist);
    if (!res.second) {
        delete new_pkt_hist;
        new_pkt_hist = *(res.first);
    }
    memcpy(state->pkt_hist, &new_pkt_hist, sizeof(PacketHistory *));
    return new_pkt_hist;
}

OpenflowUpdateState *Model::get_openflow_update_state() const {
    OpenflowUpdateState *update_state;
    memcpy(&update_state, state->openflow_update_state,
           sizeof(OpenflowUpdateState *));
    return update_state;
}

OpenflowUpdateState *
Model::set_openflow_update_state(OpenflowUpdateState &&update_state) const {
    OpenflowUpdateState *new_state =
        new OpenflowUpdateState(std::move(update_state));
    auto res = storage.openflow_update_state_hist.insert(new_state);
    if (!res.second) {
        delete new_state;
        new_state = *(res.first);
    }
    memcpy(state->openflow_update_state, &new_state,
           sizeof(OpenflowUpdateState *));
    return new_state;
}

bool Model::get_violated() const {
    return state->violated;
}

bool Model::set_violated(bool violated) const {
    return (state->violated = violated);
}

int Model::get_correlated_policy_idx() const {
    return state->correlated_policy_idx;
}

int Model::set_correlated_policy_idx(int correlated_policy_idx) const {
    return state->correlated_policy_idx = correlated_policy_idx;
}

ReachCounts *Model::get_reach_counts() const {
    ReachCounts *reach_counts;
    memcpy(&reach_counts, state->reach_counts, sizeof(ReachCounts *));
    return reach_counts;
}

ReachCounts *Model::set_reach_counts(ReachCounts &&reach_counts) const {
    ReachCounts *new_reach_counts = new ReachCounts(std::move(reach_counts));
    auto res = storage.reach_counts_hist.insert(new_reach_counts);
    if (!res.second) {
        delete new_reach_counts;
        new_reach_counts = *(res.first);
    }
    memcpy(state->reach_counts, &new_reach_counts, sizeof(ReachCounts *));
    return new_reach_counts;
}
