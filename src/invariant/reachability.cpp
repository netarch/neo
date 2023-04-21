#include "invariant/reachability.hpp"

#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

using namespace std;

string Reachability::to_string() const {
    string ret = "Reachability (";
    ret += reachable ? "O" : "X";
    ret += "): [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void Reachability::init() {
    Invariant::init();
    model.set_violated(false);
}

int Reachability::check_violation() {
    bool reached;
    int mode = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if ((mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) ||
        (mode == fwd_mode::FIRST_FORWARD && PS_REQ_ACK_OR_REP(proto_state))) {
        reached = (target_nodes.count(model.get_rx_node()) > 0);
    } else if (mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        // If the packet hasn't been accepted or dropped, there is nothing to
        // check.
        return POL_NULL;
    }

    model.set_violated(reachable != reached);
    model.set_choice_count(0);
    return POL_NULL;
}
