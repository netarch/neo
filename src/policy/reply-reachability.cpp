#include "policy/reply-reachability.hpp"

#include "node.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string ReplyReachabilityPolicy::to_string() const
{
    std::string ret = "reply-reachability " + comms[0].start_nodes_str();
    ret += " -> [";
    for (Node *node : query_nodes) {
        ret += " " + node->to_string();
    }
    if (reachable) {
        ret += " ] ---> original sender";
    } else {
        ret += " ] -X-> original sender";
    }
    return ret;
}

void ReplyReachabilityPolicy::init(State *state)
{
    set_violated(state, false);
    set_comm(state, 0);
    set_num_comms(state, 1);
}

int ReplyReachabilityPolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);
    int pkt_state = get_pkt_state(state);

    if ((PS_IS_TCP(pkt_state) && pkt_state < PS_TCP_L7_REP) ||
            (PS_IS_UDP(pkt_state) && pkt_state < PS_UDP_REP) ||
            (PS_IS_ICMP_ECHO(pkt_state) && pkt_state < PS_ICMP_ECHO_REP)) {
        // request
        Node *rx_node = get_rx_node(state);
        if ((mode == fwd_mode::ACCEPTED && query_nodes.count(rx_node) == 0)
                || mode == fwd_mode::DROPPED) {
            // if the request is accepted by a wrong node or dropped
            state->violated = reachable;
            state->choice_count = 0;
        } else {
            // If the request (or session construction packets) hasn't been
            // accepted or dropped, there is nothing to check.
            return POL_NULL;
        }
    } else if (PS_IS_REPLY(pkt_state)) {
        // reply
        bool reached;
        if (mode == fwd_mode::ACCEPTED) {
            reached = (get_pkt_location(state) == get_tx_node(state));
        } else if (mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            // If the reply hasn't been accepted or dropped, there is nothing to
            // check.
            return POL_NULL;
        }
        state->violated = (reachable != reached);
        state->choice_count = 0;
    }

    return POL_NULL;
}
