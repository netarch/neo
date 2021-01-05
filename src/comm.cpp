#include "comm.hpp"

#include <cctype>
#include <cassert>

#include <algorithm>
#include <regex>

#include "lib/logger.hpp"
#include "packet.hpp"
#include "model.h"

int Communication::get_protocol() const
{
    return protocol;
}

const std::vector<Node *>& Communication::get_start_nodes() const
{
    return start_nodes;
}

std::string Communication::start_nodes_str() const
{
    std::string ret = "[";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

uint16_t Communication::get_src_port(State *state) const
{
    bool is_reply = PS_IS_REPLY(state->comm_state[state->comm].pkt_state);

    if (is_reply) {
        return rx_port;
    } else {
        return tx_port;
    }
}

uint16_t Communication::get_dst_port(State *state) const
{
    bool is_reply = PS_IS_REPLY(state->comm_state[state->comm].pkt_state);

    if (is_reply) {
        return tx_port;
    } else {
        return rx_port;
    }
}

uint16_t Communication::get_tx_port() const
{
    return tx_port;
}

uint16_t Communication::get_rx_port() const
{
    return rx_port;
}

void Communication::set_tx_port(uint16_t port)
{
    tx_port = port;
}

void Communication::set_rx_port(uint16_t port)
{
    rx_port = port;
}

void Communication::compute_ecs(const EqClasses& all_ECs,
                                const EqClasses& owned_ECs)
{
    if (owned_dst_only) {
        ECs.add_mask_range(pkt_dst, owned_ECs);
    } else {
        ECs.add_mask_range(pkt_dst, all_ECs);
    }
}

void Communication::add_ec(const IPv4Address& addr)
{
    ECs.add_ec(addr);
}

EqClass *Communication::find_ec(const IPv4Address& addr) const
{
    return ECs.find_ec(addr);
}

size_t Communication::num_ecs() const
{
    return ECs.size();
}

size_t Communication::num_start_nodes() const
{
    return start_nodes.size();
}

size_t Communication::num_comms() const
{
    return ECs.size() * start_nodes.size();
}

bool Communication::set_initial_ec()
{
    if (!initial_ec || ++ECs_itr == ECs.end()) {
        ECs_itr = ECs.begin();
        initial_ec = *ECs_itr;
        assert(initial_ec != nullptr);
        return false;
    }
    initial_ec = *ECs_itr;
    assert(initial_ec != nullptr);
    return true;
}

EqClass *Communication::get_initial_ec() const
{
    return initial_ec;
}

bool operator==(const Communication& a, const Communication& b)
{
    if (a.protocol == b.protocol &&
            a.start_nodes == b.start_nodes &&
            a.tx_port == b.tx_port &&
            a.rx_port == b.rx_port &&
            *a.initial_ec == *b.initial_ec) {
        return true;
    }
    return false;
}
