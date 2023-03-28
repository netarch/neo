#include "invariant/loadbalance.hpp"

#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"
#include "reachcounts.hpp"

using namespace std;

string LoadBalance::to_string() const {
    string ret = "Loadbalance (max_dispersion_index: " +
                 std::to_string(max_dispersion_index) + "): [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void LoadBalance::init() {
    Invariant::init();
    model.set_violated(false);
    model.set_reach_counts(ReachCounts());
}

static inline double
compute_dispersion_index(const unordered_set<Node *> &target_nodes,
                         const ReachCounts &reach_counts) {
    double mean = 0;
    for (Node *node : target_nodes) {
        mean += reach_counts[node];
    }
    mean /= target_nodes.size();
    logger.debug("(compute_dispersion_index) mean: " + to_string(mean));

    double variance = 0;
    for (Node *node : target_nodes) {
        variance += (reach_counts[node] - mean) * (reach_counts[node] - mean);
    }
    variance /= target_nodes.size();
    logger.debug("(compute_dispersion_index) variance: " + to_string(variance));

    double index = variance / mean;
    logger.debug("(compute_dispersion_index) index: " + to_string(index));
    return index;
}

int LoadBalance::check_violation() {
    int mode = model.get_fwd_mode();
    int proto_state = model.get_proto_state();

    if (mode == fwd_mode::ACCEPTED && PS_IS_FIRST(proto_state)) {
        Node *rx_node = model.get_rx_node();
        ReachCounts new_reach_counts(*model.get_reach_counts());
        new_reach_counts.increase(rx_node);
        // logger.debug(new_reach_counts.to_string());

        if (target_nodes.count(rx_node)) {
            double current_dispersion_index =
                compute_dispersion_index(target_nodes, new_reach_counts);
            if (current_dispersion_index > max_dispersion_index) {
                logger.info("Current dispersion index: " +
                            std::to_string(current_dispersion_index));
                model.set_violated(true); // violation
                model.set_choice_count(0);
            }
        }
        model.set_reach_counts(std::move(new_reach_counts));

        // so that the connection won't be chosen
        model.set_executable(0);
    }

    return POL_NULL;
}
