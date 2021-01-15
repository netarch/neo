#include "policy/reachability.hpp"

#include <cstring>

#include "process/forwarding.hpp"
#include "model.h"

std::string ReachabilityPolicy::to_string() const
{
    std::string ret = "reachability " + comms[0].start_nodes_str();
    if (reachable) {
        ret += " ---> [";
    } else {
        ret += " -X-> [";
    }
    for (Node *node : final_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void ReachabilityPolicy::init(State *state)
{
    state->violated = false;
}

int ReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (mode == fwd_mode::ACCEPTED &&
            (pkt_state == PS_HTTP_REQ || pkt_state == PS_ICMP_ECHO_REQ)) {
        Node *rx_node;
        memcpy(&rx_node, state->comm_state[state->comm].rx_node,
               sizeof(Node *));
        reached = (final_nodes.count(rx_node) > 0);
    } else if (mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        /*
         * If the packet hasn't been accepted or dropped, there is nothing to
         * check.
         */
        return POL_NULL;
    }

    state->violated = (reachable != reached);
    state->choice_count = 0;

    return POL_NULL;
}
