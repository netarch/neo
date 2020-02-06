#include "policy/loadbalance.hpp"

#include <iostream>
#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

LoadBalancePolicy::LoadBalancePolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy()
{
    parse_protocol(config);
    parse_pkt_dst(config);
    parse_owned_dst_only(config);
    parse_start_node(config, net);
    parse_tcp_ports(config);
    parse_final_node(config, net);
}

LoadBalancePolicy::~LoadBalancePolicy()
{
}

void LoadBalancePolicy::parse_final_node(
    const std::shared_ptr<cpptoml::table>& config, const Network& net)
{
    auto final_regex = config->get_as<std::string>("final_node");

    if (!final_regex) {
        Logger::get().err("Missing final node");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.insert(node.second);
        }
    }
}

std::string LoadBalancePolicy::to_string() const
{
    std::string ret = "loadbalance [";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }

    ret += " ] --> [";

    for (Node *node : final_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void LoadBalancePolicy::init(State *state) const
{
    state->violated = false;
}

void LoadBalancePolicy::check_violation(State *state)
{
    state->violated = false;
    if(state->choice_count == 0) {
        //this is the final invocation
        if (visited.size() != final_nodes.size()) {
            state->violated = true;
        }
        
        return;
    }

    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (mode == fwd_mode::ACCEPTED &&
            (pkt_state == PS_HTTP_REQ || pkt_state == PS_ICMP_ECHO_REQ)) {
        if (final_nodes.count(comm_rx) && !visited.count(comm_rx)) {
            visited.insert(comm_rx);
            state->choice_count = 0;
        }
    }
}
