#include "reachcounts.hpp"

#include <stdexcept>

#include "node.hpp"
#include "lib/hash.hpp"

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

int ReachCounts::total_counts() const
{
    int num = 0;
    for (const auto& pair : this->counts) {
        num += pair.second;
    }
    return num;
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
