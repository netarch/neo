#include "policy/loadbalance.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

LoadBalancePolicy::LoadBalancePolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    auto final_regex = config->get_as<std::string>("final_node");
    auto repeat = config->get_as<int>("repeat");
    auto comm_cfg = config->get_table("communication");

    if (!final_regex) {
        Logger::get().err("Missing final node");
    }
    if (!comm_cfg) {
        Logger::get().err("Missing communication");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.insert(node.second);
        }
    }

    if (repeat) {
        repetition = *repeat;
    } else {
        repetition = final_nodes.size();
    }

    Communication comm(comm_cfg, net);
    comms.push_back(std::move(comm));
}

std::string LoadBalancePolicy::to_string() const
{
    std::string ret = "loadbalance " + comms[0].start_nodes_str() + " --> [";
    for (Node *node : final_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void LoadBalancePolicy::init(State *state)
{
    state->violated = true;
    state->comm_state[state->comm].repetition = 0;
    visited.clear();
}

int LoadBalancePolicy::check_violation(State *state)
{
    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (mode == fwd_mode::ACCEPTED &&
            (pkt_state == PS_TCP_INIT_1 || pkt_state == PS_ICMP_ECHO_REQ)) {
        Node *rx_node;
        memcpy(&rx_node, state->comm_state[state->comm].rx_node,
               sizeof(Node *));
        if (final_nodes.count(rx_node)) {
            visited.insert(rx_node);
        }
        state->choice_count = 0;
    }

    if (state->choice_count == 0) {
        ++state->comm_state[state->comm].repetition;
        if (visited.size() == final_nodes.size()) {
            state->violated = false;
            Logger::get().info("LBPolicy: repeated " +
                               std::to_string(state->comm_state[state->comm].repetition) +
                               " times");
        } else if (state->comm_state[state->comm].repetition < repetition) {
            // see each repetition as a different communication
            comms[0].set_tx_port(comms[0].get_tx_port() + 1);
            state->choice_count = 1;
            // reset the forwarding process and repeat
            return POL_RESET_FWD;
        }
    }

    return POL_NULL;
}
