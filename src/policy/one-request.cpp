#include "policy/one-request.hpp"

#include "node.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string OneRequestPolicy::to_string() const
{
    std::string ret = "one-request among:\n";
    for (const Communication& comm : comms) {
        ret += comm.start_nodes_str() + " ---> [";
        for (Node *node : server_nodes) {
            ret += " " + node->to_string();
        }
        ret += " ]\n";
    }
    ret.pop_back();
    return ret;
}

void OneRequestPolicy::init(State *state)
{
    set_violated(state, true);
    set_comm(state, 0);
    set_num_comms(state, this->comms.size());
}

// TODO
int OneRequestPolicy::check_violation(State *state __attribute__((unused)))
{
    //bool reached;
    //int mode = state->comm_state[state->comm].fwd_mode;
    //uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    //if (mode == fwd_mode::ACCEPTED &&
    //        (pkt_state == PS_TCP_L7_REQ || pkt_state == PS_ICMP_ECHO_REQ)) {
    //    Node *rx_node;
    //    memcpy(&rx_node, state->comm_state[state->comm].rx_node,
    //           sizeof(Node *));
    //    reached = (final_nodes.count(rx_node) > 0);
    //} else if (mode == fwd_mode::DROPPED) {
    //    reached = false;
    //} else {
    //    /*
    //     * If the packet hasn't been accepted or dropped, there is nothing to
    //     * check.
    //     */
    //    return POL_NULL;
    //}

    //state->violated = (reachable != reached);
    //state->choice_count = 0;

    return POL_NULL;
}
