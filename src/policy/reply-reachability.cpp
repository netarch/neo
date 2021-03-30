#include "policy/reply-reachability.hpp"

#include "node.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string ReplyReachabilityPolicy::to_string() const
{
    std::string ret = "Reply-reachability:\n"
                      "\treachable: " + std::to_string(reachable) + "\n"
                      "\ttarget_nodes: [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]\n\t" + conns_str();
    return ret;
}

void ReplyReachabilityPolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, false);
}

int ReplyReachabilityPolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);
    int proto_state = get_proto_state(state);

    if ((PS_IS_TCP(proto_state) && proto_state < PS_TCP_L7_REP) ||
            (PS_IS_UDP(proto_state) && proto_state < PS_UDP_REP) ||
            (PS_IS_ICMP_ECHO(proto_state) && proto_state < PS_ICMP_ECHO_REP)) {
        // request
        Node *rx_node = get_rx_node(state);
        if ((mode == fwd_mode::ACCEPTED && target_nodes.count(rx_node) == 0)
                || mode == fwd_mode::DROPPED) {
            // if the request is accepted by a wrong node or dropped
            state->violated = reachable;
            state->choice_count = 0;
        } else {
            // If the request (or session construction packets) hasn't been
            // accepted or dropped, there is nothing to check.
            return POL_NULL;
        }
    } else if (PS_IS_REPLY(proto_state)) {
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
