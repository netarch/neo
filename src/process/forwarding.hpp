#pragma once

#include <vector>

#include "process/process.hpp"
#include "node.hpp"

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

    std::vector<Node *> start_nodes;

public:
    ForwardingProcess();

    void init(State *, const std::vector<Node *>&);
    void exec_step(State *) const override;
};
