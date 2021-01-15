#include "policy/loadbalance.hpp"

#include <cstring>

#include "process/forwarding.hpp"
#include "model.h"

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
