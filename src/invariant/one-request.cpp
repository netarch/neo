#include "invariant/one-request.hpp"

#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"
#include "reachcounts.hpp"

using namespace std;

string OneRequest::to_string() const {
    string ret = "OneRequest: [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void OneRequest::init() {
    Invariant::init();
    model.set_violated(true);
    model.set_reach_counts(ReachCounts());
}

int OneRequest::check_violation() {
    int mode = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) {
        Node *rx_node = model.get_rx_node();
        if (target_nodes.count(rx_node)) {
            ReachCounts new_reach_counts(*model.get_reach_counts());
            new_reach_counts.increase(rx_node);
            if (new_reach_counts.total_counts() > 1) {
                model.set_violated(true); // violation
                model.set_choice_count(0);
            }
            model.set_reach_counts(std::move(new_reach_counts));

            // so that the connection won't be chosen
            model.set_executable(0);
        }
    }

    return POL_NULL;
}
