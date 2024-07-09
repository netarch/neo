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

static inline bool request_is_accepted(int mode, int proto_state) {
    return (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) ||
           (mode == fwd_mode::FIRST_FORWARD && PS_REQ_ACK_OR_REP(proto_state));
}

static inline bool packet_is_dropped(int mode) {
    return mode == fwd_mode::DROPPED;
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

int Reachability::check_violation() {
    bool reached;
    int mode        = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if (request_is_accepted(mode, proto_state) &&
        (target_nodes.count(model.get_rx_node()) > 0)) {
        // The packet is accepted by one of the target nodes.
        reached = true;
    } else if ((request_is_accepted(mode, proto_state) ||
                packet_is_dropped(mode)) &&
               !other_executable_conns_exist(model)) {
        // The current connection is accepted by a non-target node, or dropped.
        // If none of the other connections are executable up to this point, the
        // target nodes are not reached.
        reached = false;
    } else {
        // Either the packet hasn't been accepted or dropped, or there are other
        // executable connections remaining. If there are other executable
        // connections, then those connections should be related to the current
        // connection (e.g., after an NAT). In either case, we continue the
        // exploration until either all connections are dropped or get accepted
        // by non-target endpoints, or some connections are accepted by one of
        // the target endpoints.
        return POL_NULL;
    }

    model.set_violated(reachable != reached);
    model.set_choice_count(0);
    return POL_NULL;
}
