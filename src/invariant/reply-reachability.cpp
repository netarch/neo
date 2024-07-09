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

static inline bool request_is_accepted(int mode, int proto_state) {
    return (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) ||
           (mode == fwd_mode::FIRST_FORWARD && PS_REQ_ACK_OR_REP(proto_state));
}

static inline bool other_executable_conns_exist(const Model &model) {
    int current_conn = model.get_conn();
    for (int conn = 0; conn < model.get_num_conns(); ++conn) {
        if (conn != current_conn && model.get_executable_for_conn(conn) > 0) {
            return true;
        }
    }
    return false;
}

int ReplyReachability::check_violation() {
    int mode        = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if ((PS_IS_TCP(proto_state) && proto_state < PS_TCP_L7_REP) ||
        (PS_IS_UDP(proto_state) && proto_state < PS_UDP_REP) ||
        (PS_IS_ICMP_ECHO(proto_state) && proto_state < PS_ICMP_ECHO_REP)) {
        // request
        if (((request_is_accepted(mode, proto_state) &&
              target_nodes.count(model.get_rx_node()) == 0) ||
             mode == fwd_mode::DROPPED) &&
            !other_executable_conns_exist(model)) {
            // The request (or session construction packets) is accepted by a
            // wrong node or dropped, while no other executable connections
            // exist.
            model.set_violated(reachable);
            model.set_choice_count(0);
        } else {
            // The request (or session construction packets) is either accepted
            // by the correct nodes, hasn't been accepted or dropped, or there
            // are other executable connections remainging. If there are other
            // executable connections, then those connections should be related
            // to the current connection (e.g., after an NAT). In either case,
            // we continue the exploration until otherwise.
            return POL_NULL;
        }
    } else if (PS_IS_REPLY(proto_state)) {
        // reply
        bool reached;
        if (mode == fwd_mode::ACCEPTED &&
            model.get_pkt_location() == model.get_tx_node()) {
            // The reply packet is accepted by the original sender.
            reached = true;
        } else if (((mode == fwd_mode::ACCEPTED &&
                     model.get_pkt_location() != model.get_tx_node()) ||
                    mode == fwd_mode::DROPPED) &&
                   !other_executable_conns_exist(model)) {
            // The reply packet is accepted by an incorrect sender or is
            // dropped, while none of the other connections are executable.
            reached = false;
        } else {
            // Either the reply packet hasn't been accepted or dropped, or there
            // are other executable connections remaining.
            return POL_NULL;
        }

        model.set_violated(reachable != reached);
        model.set_choice_count(0);
    }

    return POL_NULL;
}
