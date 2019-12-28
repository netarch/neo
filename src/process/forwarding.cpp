#include <cstring>
#include <typeinfo>

#include "process/forwarding.hpp"
#include "fib.hpp"
#include "lib/hash.hpp"

ForwardingProcess::~ForwardingProcess()
{
    for (auto& candidates : candidates_hist) {
        delete candidates;
    }
    for (auto& node_pkt_hist : node_pkt_hist_hist) {
        delete node_pkt_hist;
    }
    for (auto& pkt_hist : pkt_hist_hist) {
        delete pkt_hist;
    }
}

void ForwardingProcess::init(State *state, const Network& network,
                             Policy *policy)
{
    this->policy = policy;

    state->comm_state[state->comm].fwd_mode = int(fwd_mode::PACKET_ENTRY);
    state->comm_state[state->comm].pkt_state = PS_TCP_INIT_1;
    memset(state->comm_state[state->comm].src_ip, 0, sizeof(uint32_t));
    memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].pkt_location, 0, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));

    // initialize packet history
    PacketHistory *pkt_hist = new PacketHistory(network);
    pkt_hist_hist.insert(pkt_hist); // there should be no duplicates
    memcpy(state->comm_state[state->comm].pkt_hist, &pkt_hist,
           sizeof(PacketHistory *));

    // update candidates as start nodes
    std::vector<FIB_IPNH> candidates;
    for (Node *start_node : policy->get_start_nodes(state)) {
        candidates.push_back(FIB_IPNH(start_node, nullptr, start_node,
                                      nullptr));
    }
    update_candidates(state, candidates);
}

void ForwardingProcess::exec_step(State *state, Network& network)
{
    if (!enabled) {
        return;
    }

    int mode = state->comm_state[state->comm].fwd_mode;
    switch (mode) {
        case fwd_mode::PACKET_ENTRY:
            packet_entry(state);
            break;
        case fwd_mode::FIRST_COLLECT:
            first_collect(state);
            break;
        case fwd_mode::FIRST_FORWARD:
            first_forward(state);
            break;
        case fwd_mode::COLLECT_NHOPS:
            collect_next_hops(state);
            break;
        case fwd_mode::FORWARD_PACKET:
            forward_packet(state);
            break;
        case fwd_mode::ACCEPTED:
            accepted(state, network);
            break;
        case fwd_mode::DROPPED:
            state->choice_count = 0;
            break;
        default:
            Logger::get().err("Forwarding process: unknown mode ("
                              + std::to_string(mode) + ")");
    }
}

void ForwardingProcess::packet_entry(State *state) const
{
    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    Node *entry = (*candidates)[state->choice].get_l3_node();
    memcpy(state->comm_state[state->comm].src_node, &entry, sizeof(Node *));
    memcpy(state->comm_state[state->comm].pkt_location, &entry, sizeof(Node *));
    memset(state->comm_state[state->comm].ingress_intf, 0, sizeof(Interface *));
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;
    Logger::get().info("Packet (state: " + std::to_string(pkt_state)
                       + ") injected at " + entry->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FIRST_COLLECT);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::first_collect(State *state)
{
    collect_next_hops(state);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FIRST_FORWARD);
}

void ForwardingProcess::first_forward(State *state) const
{
    // update the source IP address
    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));
    FIB_IPNH next_hop = (*candidates)[state->choice];
    if (next_hop.get_l2_intf()) {
        const Interface *egress_intf
            = next_hop.get_l2_node()->get_peer(
                  next_hop.get_l2_intf()->get_name()
              ).second;
        uint32_t src_ip = egress_intf->addr().get_value();
        memcpy(state->comm_state[state->comm].src_ip, &src_ip,
               sizeof(uint32_t));
    }

    forward_packet(state);
}

void ForwardingProcess::collect_next_hops(State *state)
{
    std::set<FIB_IPNH> next_hops;

    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    if (typeid(*current_node) == typeid(Node)) {
        // current_node is a Node; look up next hops from the fib
        FIB *fib;
        memcpy(&fib, state->comm_state[state->comm].fib, sizeof(FIB *));
        next_hops = fib->lookup(current_node);
    } else {
        // current_node is a Middlebox; inject packet to get next hops
        EqClass *cur_ec;
        memcpy(&cur_ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
        next_hops = inject_packet(state, (Middlebox *)current_node,
                                  cur_ec->begin()->get_lb());
    }

    if (next_hops.empty()) {
        Logger::get().info("Packet dropped by " + current_node->to_string());
        state->comm_state[state->comm].fwd_mode = int(fwd_mode::DROPPED);
        state->choice_count = 0;
        return;
    }
    std::vector<FIB_IPNH> candidates;
    for (const FIB_IPNH& next_hop : next_hops) {
        candidates.push_back(next_hop);
    }
    update_candidates(state, candidates);
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::FORWARD_PACKET);
}

void ForwardingProcess::forward_packet(State *state) const
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));

    std::vector<FIB_IPNH> *candidates;
    memcpy(&candidates, state->candidates, sizeof(std::vector<FIB_IPNH> *));

    Node *next_hop = (*candidates)[state->choice].get_l3_node();
    if (next_hop == current_node) {
        Logger::get().info("Packet delivered at " + next_hop->to_string());
        state->comm_state[state->comm].fwd_mode = int(fwd_mode::ACCEPTED);
        state->choice_count = 1;
        return;
    }
    memcpy(state->comm_state[state->comm].pkt_location, &next_hop,
           sizeof(Node *));

    Interface *ingress_intf = (*candidates)[state->choice].get_l3_intf();
    memcpy(state->comm_state[state->comm].ingress_intf, &ingress_intf,
           sizeof(Interface *));

    Logger::get().info("Packet forwarded to " + next_hop->to_string() + ", "
                       + ingress_intf->to_string());
    state->comm_state[state->comm].fwd_mode = int(fwd_mode::COLLECT_NHOPS);
    state->choice_count = 1;    // deterministic choice
}

void ForwardingProcess::accepted(State *state, Network& network)
{
    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));
    uint32_t src_ip;
    memcpy(&src_ip, state->comm_state[state->comm].src_ip, sizeof(uint32_t));

    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;
    switch (pkt_state) {
        case PS_TCP_INIT_1: {
            state->comm_state[state->comm].pkt_state = PS_TCP_INIT_2;
            state->comm_state[state->comm].fwd_mode = fwd_mode::PACKET_ENTRY;
            memset(state->comm_state[state->comm].src_ip, 0, sizeof(uint32_t));
            memset(state->comm_state[state->comm].src_node, 0, sizeof(Node *));
            memset(state->comm_state[state->comm].ingress_intf, 0,
                   sizeof(Interface *));

            // update candidates as start nodes
            std::vector<FIB_IPNH> candidates(
                1, FIB_IPNH(current_node, nullptr, current_node, nullptr));
            update_candidates(state, candidates);

            // update packet EC and reinitialize the FIB
            EqClass *next_ec = policy->get_ecs().find_ec(src_ip);
            memcpy(state->comm_state[state->comm].ec, &next_ec,
                   sizeof(EqClass *));
            network.update_fib(state);
        }
        break;
        case PS_TCP_INIT_2:
            break;
        case PS_TCP_INIT_3:
            break;
        case PS_HTTP_REQ:
            break;
        case PS_HTTP_REQ_A:
            break;
        case PS_HTTP_REP:
            break;
        case PS_HTTP_REP_A:
            break;
        case PS_TCP_TERM_1:
            break;
        case PS_TCP_TERM_2:
            break;
        case PS_TCP_TERM_3:
            break;
        case PS_ICMP_REQ:
            break;
        case PS_ICMP_REP:
            break;
        default:
            Logger::get().err("Forwarding process: unknown packet state "
                              + std::to_string(pkt_state));
    }
}

std::set<FIB_IPNH> ForwardingProcess::inject_packet(
    State *state, Middlebox *mb, const IPv4Address& dst_ip)
{
    // check state's pkt_hist and rewind the middlebox state
    PacketHistory *pkt_hist;
    memcpy(&pkt_hist, state->comm_state[state->comm].pkt_hist,
           sizeof(PacketHistory *));
    NodePacketHistory *current_nph = pkt_hist->get_node_pkt_hist(mb);
    mb->rewind(current_nph);

    // construct new packet
    Interface *ingress_intf;
    memcpy(&ingress_intf, state->comm_state[state->comm].ingress_intf,
           sizeof(Interface *));
    uint32_t src_ip;
    memcpy(&src_ip, state->comm_state[state->comm].src_ip, sizeof(uint32_t));
    Packet *new_pkt = new Packet(ingress_intf, src_ip, dst_ip,
                                 policy->get_src_port(state),
                                 policy->get_dst_port(state),
                                 state->comm_state[state->comm].pkt_state);
    auto res1 = all_pkts.insert(new_pkt);
    if (!res1.second) {
        delete new_pkt;
        new_pkt = *(res1.first);
    }

    // update node_pkt_hist with this new packet
    NodePacketHistory *new_nph = new NodePacketHistory(new_pkt, current_nph);
    auto res2 = node_pkt_hist_hist.insert(new_nph);
    if (!res2.second) {
        delete new_nph;
        new_nph = *(res2.first);
    }
    mb->set_node_pkt_hist(new_nph);

    // update pkt_hist with this new packet
    PacketHistory *new_pkt_hist = new PacketHistory(*pkt_hist);
    new_pkt_hist->set_node_pkt_hist(mb, new_nph);
    auto res3 = pkt_hist_hist.insert(new_pkt_hist);
    if (!res3.second) {
        delete new_pkt_hist;
        new_pkt_hist = *(res3.first);
    }
    memcpy(state->comm_state[state->comm].pkt_hist, &new_pkt_hist,
           sizeof(PacketHistory *));

    // inject packet
    return mb->send_pkt(*new_pkt);
}

void ForwardingProcess::update_candidates(
    State *state,
    const std::vector<FIB_IPNH>& new_candidates)
{
    std::vector<FIB_IPNH> *candidates
        = new std::vector<FIB_IPNH>(new_candidates);
    auto res = candidates_hist.insert(candidates);
    if (!res.second) {
        delete candidates;
        candidates = *(res.first);
    }
    state->choice_count = candidates->size();
    memcpy(state->candidates, &candidates, sizeof(std::vector<FIB_IPNH> *));
}

size_t CandHash::operator()(const std::vector<FIB_IPNH> *const& c) const
{
    return hash::hash(c->data(), c->size() * sizeof(FIB_IPNH));
}

bool CandEq::operator()(const std::vector<FIB_IPNH> *const& a,
                        const std::vector<FIB_IPNH> *const& b) const
{
    if (a->size() != b->size()) {
        return false;
    }
    return memcmp(a->data(), b->data(), a->size() * sizeof(FIB_IPNH)) == 0;
}
