#include "policy/reachability.hpp"

#include "node.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
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
    set_violated(state, false);
    set_comm(state, 0);
    set_num_comms(state, 1);
}

int ReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    int mode = get_fwd_mode(state);
    int pkt_state = get_pkt_state(state);

    if (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(pkt_state)) {
        reached = (final_nodes.count(get_rx_node(state)) > 0);
    } else if (mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        // If the packet hasn't been accepted or dropped, there is nothing to
        // check.
        return POL_NULL;
    }

    state->violated = (reachable != reached);
    state->choice_count = 0;
    return POL_NULL;
}
