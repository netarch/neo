#include "policy/multicomm-lb.hpp"

#include <stdexcept>

#include "node.hpp"
#include "packet.hpp"
#include "process/forwarding.hpp"
#include "lib/hash.hpp"
#include "model-access.hpp"
#include "model.h"

std::string MulticommLBPolicy::to_string() const
{
    std::string ret = "multicomm-lb (max dispersion index: " +
                      std::to_string(max_dispersion_index) + "):\n";
    for (const Communication& comm : comms) {
        ret += comm.start_nodes_str() + " ---> [";
        for (Node *node : final_nodes) {
            ret += " " + node->to_string();
        }
        ret += " ]\n";
    }
    ret.pop_back();
    return ret;
}

void MulticommLBPolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, false);
}

static inline double compute_dispersion_index(
    const std::unordered_set<Node *>& final_nodes,
    const ReachCounts& reach_counts)
{
    double mean = 0;
    for (Node *node : final_nodes) {
        mean += reach_counts[node];
    }
    mean /= final_nodes.size();
    Logger::debug("(compute_dispersion_index) mean: " + std::to_string(mean));

    double variance = 0;
    for (Node *node : final_nodes) {
        variance += (reach_counts[node] - mean) * (reach_counts[node] - mean);
    }
    variance /= final_nodes.size();
    Logger::debug("(compute_dispersion_index) variance: " + std::to_string(variance));

    double index = variance / mean;
    Logger::debug("(compute_dispersion_index) index: " + std::to_string(index));
    return index;
}

int MulticommLBPolicy::check_violation(State *state)
{

    int mode = get_fwd_mode(state);
    int pkt_state = get_pkt_state(state);

    if (mode == fwd_mode::ACCEPTED && PS_IS_FIRST(pkt_state)) {
        Node *rx_node = get_rx_node(state);
        ReachCounts new_reach_counts(*get_reach_counts(state));
        new_reach_counts.increase(rx_node);

        if (final_nodes.count(rx_node)) {
            double current_dispersion_index
                = compute_dispersion_index(final_nodes, new_reach_counts);
            if (current_dispersion_index > max_dispersion_index) {
                set_violated(state, true); // violation
                Logger::info("Current dispersion index: "
                             + std::to_string(current_dispersion_index));
            }
        }
        set_reach_counts(state, std::move(new_reach_counts));

        // so that the communication won't be chosen
        set_pkt_state(state, PS_LAST_PKT_STATE(pkt_state));
    }

    return POL_NULL;
}

/******************************************************************************/

std::string ReachCounts::to_string() const
{
    std::string ret = "Current reach counts:";
    for (const auto& pair : this->counts) {
        ret += "\n\t" + pair.first->to_string() + ": " + std::to_string(pair.second);
    }
    return ret;
}

int ReachCounts::operator[](Node *const& node) const
{
    try {
        return this->counts.at(node);
    } catch (const std::out_of_range& err) {
        return 0;
    }
}

void ReachCounts::increase(Node *const& node)
{
    this->counts[node]++;
}

size_t ReachCountsHash::operator()(const ReachCounts *const& rc) const
{
    size_t value = 0;
    std::hash<Node *> node_hf;
    std::hash<int> int_hf;
    for (const auto& entry : rc->counts) {
        hash::hash_combine(value, node_hf(entry.first));
        hash::hash_combine(value, int_hf(entry.second));
    }
    return value;
}

bool ReachCountsEq::operator()(const ReachCounts *const& a,
                               const ReachCounts *const& b) const
{
    return a->counts == b->counts;
}
