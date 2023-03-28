#include "invariant/reply-reachability.hpp"

#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

using namespace std;

string ReplyReachability::to_string() const {
    string ret = "ReplyReachability (";
    ret += reachable ? "O" : "X";
    ret += "): [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void ReplyReachability::init() {
    Invariant::init();
    model.set_violated(false);
}

int ReplyReachability::check_violation() {
    int mode = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if ((PS_IS_TCP(proto_state) && proto_state < PS_TCP_L7_REP) ||
        (PS_IS_UDP(proto_state) && proto_state < PS_UDP_REP) ||
        (PS_IS_ICMP_ECHO(proto_state) && proto_state < PS_ICMP_ECHO_REP)) {
        // request
        Node *rx_node = model.get_rx_node();
        if ((mode == fwd_mode::ACCEPTED && target_nodes.count(rx_node) == 0) ||
            mode == fwd_mode::DROPPED) {
            // if the request is accepted by a wrong node or dropped
            model.set_violated(reachable);
            model.set_choice_count(0);
        } else {
            // If the request (or session construction packets) hasn't been
            // accepted or dropped, there is nothing to check.
            return POL_NULL;
        }
    } else if (PS_IS_REPLY(proto_state)) {
        // reply
        bool reached;
        if (mode == fwd_mode::ACCEPTED) {
            reached = (model.get_pkt_location() == model.get_tx_node());
        } else if (mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            // If the reply hasn't been accepted or dropped, there is nothing to
            // check.
            return POL_NULL;
        }
        model.set_violated(reachable != reached);
        model.set_choice_count(0);
    }

    return POL_NULL;
}
