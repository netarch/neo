#include "policy/loadbalance.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

LoadBalancePolicy::LoadBalancePolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    parse_protocol(config);
    parse_pkt_dst(config);
    parse_owned_dst_only(config);
    parse_start_node(config, net);
    parse_tcp_ports(config);
    parse_final_node(config, net);
    parse_repeat(config);
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

void LoadBalancePolicy::parse_repeat(
    const std::shared_ptr<cpptoml::table>& config)
{
    auto repeat = config->get_as<int>("repeat");

    if (repeat) {
        repetition = *repeat;
    } else {
        repetition = final_nodes.size();
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
    state->violated = true;
    state->comm_state[state->comm].repetition = 0;
}

void LoadBalancePolicy::check_violation(State *state)
{
    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (mode == fwd_mode::ACCEPTED &&
            (pkt_state == PS_TCP_INIT_1 || pkt_state == PS_ICMP_ECHO_REQ)) {
        if (final_nodes.count(comm_rx)) {
            visited.insert(comm_rx);
        }
        state->choice_count = 0;
    }

    if (state->choice_count == 0) {
        if (visited.size() == final_nodes.size()) {
            state->violated = false;
            return;
        }
        if (++state->comm_state[state->comm].repetition < repetition) {
            state->choice_count = 1;
        }
    }
}
