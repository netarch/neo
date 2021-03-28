#include "policy/loadbalance.hpp"

#include "node.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
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

void LoadBalancePolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, true);

    visited.clear();
}

int LoadBalancePolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);
    int proto_state = get_proto_state(state);

    if (mode == fwd_mode::ACCEPTED && PS_IS_FIRST(proto_state)) {
        Node *rx_node = get_rx_node(state);
        if (final_nodes.count(rx_node)) {
            visited.insert(rx_node);
        }
        state->choice_count = 0;
    }

    if (state->choice_count == 0) {
        set_repetition(state, get_repetition(state) + 1);
        if (visited.size() == final_nodes.size()) {
            set_violated(state, false);
            Logger::info("Repeated " + std::to_string(get_repetition(state))
                         + " times");
        } else if (get_repetition(state) < repetition) {
            // see each repetition as a different communication
            //comms[0].set_src_port(get_src_port(state) + 1);
            // reset the forwarding process and repeat
            //return POL_RESET_FWD;
        }
    }

    return POL_NULL;
}
