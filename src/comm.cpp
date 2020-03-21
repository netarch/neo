#include "comm.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

#include "lib/logger.hpp"
#include "packet.hpp"
#include "model.h"

Communication::Communication(const std::shared_ptr<cpptoml::table>& config,
                             const Network& net)
    : initial_ec(nullptr)
{
    auto proto_str = config->get_as<std::string>("protocol");
    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");
    auto owned_only = config->get_as<bool>("owned_dst_only");
    auto start_regex = config->get_as<std::string>("start_node");

    if (!proto_str) {
        Logger::get().err("Missing protocol");
    }
    if (!pkt_dst_str) {
        Logger::get().err("Missing packet destination");
    }
    if (!start_regex) {
        Logger::get().err("Missing start node");
    }

    std::string proto_s = *proto_str;
    std::transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
    [](unsigned char c) {
        return std::tolower(c);
    });
    if (proto_s == "http") {
        protocol = proto::PR_HTTP;
    } else if (proto_s == "icmp-echo") {
        protocol = proto::PR_ICMP_ECHO;
    } else {
        Logger::get().err("Unknown protocol: " + *proto_str);
    }

    std::string dst_str = *pkt_dst_str;
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    pkt_dst = IPRange<IPv4Address>(dst_str);

    owned_dst_only = owned_only ? *owned_only : true;

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
    }

    // NOTE: fixed port numbers for now
    tx_port = 1234;
    rx_port = 80;
}

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
        return false;
    }
    initial_ec = *ECs_itr;
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
