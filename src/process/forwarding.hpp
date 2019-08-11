#pragma once

#include <boost/functional/hash.hpp>
#include "process/process.hpp"
#include "node.hpp"
#include "lib/ip.hpp"

class ForwardingProcess : public Process
{
private:
    Node *current_node;

    /*
     * If ingress_intf is not a L2 switchport, then ingress_intf and l3_nhop
     * should be nullptr. They should not affect the state.
     */
    Interface *ingress_intf;
    Node *l3_nhop;

public:
    static std::unordered_set<std::vector<Node *>> selected_nodes_set;
    ForwardingProcess();

    void init();
    void exec_step(State *state) const override;
    static const std::vector<Node *> *get_or_create_selection_list(std::vector<Node *>&&);
};

namespace std
{
template <>
struct hash<std::vector<Node *>> {
    size_t operator()(const vector<Node *>& v) const
    {
        size_t seed = 0;
        for (const Node *node : v) {
            boost::hash_combine(seed, node);
        }

        return seed;
    }
};
}
