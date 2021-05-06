#include "policy/loadbalance.hpp"

#include "node.hpp"
#include "reachcounts.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string LoadBalancePolicy::to_string() const
{
    std::string ret = "Loadbalance (max_dispersion_index: "
                      + std::to_string(max_dispersion_index) + "): [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

void LoadBalancePolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, false);
    ReachCounts reach_counts;
    set_reach_counts(state, std::move(reach_counts));
}

static inline double compute_dispersion_index(
    const std::unordered_set<Node *>& target_nodes,
    const ReachCounts& reach_counts)
{
    double mean = 0;
    for (Node *node : target_nodes) {
        mean += reach_counts[node];
    }
    mean /= target_nodes.size();
    Logger::debug("(compute_dispersion_index) mean: " + std::to_string(mean));

    double variance = 0;
    for (Node *node : target_nodes) {
        variance += (reach_counts[node] - mean) * (reach_counts[node] - mean);
    }
    variance /= target_nodes.size();
    Logger::debug("(compute_dispersion_index) variance: " + std::to_string(variance));

    double index = variance / mean;
    Logger::debug("(compute_dispersion_index) index: " + std::to_string(index));
    return index;
}

int LoadBalancePolicy::check_violation(State *state)
{
    int mode = get_fwd_mode(state);
    int proto_state = get_proto_state(state);

    if (mode == fwd_mode::ACCEPTED && PS_IS_FIRST(proto_state)) {
        Node *rx_node = get_rx_node(state);
        ReachCounts new_reach_counts(*get_reach_counts(state));
        new_reach_counts.increase(rx_node);
        //Logger::debug(new_reach_counts.to_string());

        if (target_nodes.count(rx_node)) {
            double current_dispersion_index
                = compute_dispersion_index(target_nodes, new_reach_counts);
            if (current_dispersion_index > max_dispersion_index) {
                Logger::info("Current dispersion index: "
                             + std::to_string(current_dispersion_index));
                set_violated(state, true); // violation
                state->choice_count = 0;
            }
        }
        set_reach_counts(state, std::move(new_reach_counts));

        // so that the connection won't be chosen
        set_executable(state, 0);
    }

    return POL_NULL;
}
