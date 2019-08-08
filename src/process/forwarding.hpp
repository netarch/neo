#pragma once

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
    ForwardingProcess();

    void init(Node *);
    void exec_step(Network&, const EqClass&) override;
};
