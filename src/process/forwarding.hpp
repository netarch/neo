#pragma once

#include "process/process.hpp"
#include "node.hpp"
#include "lib/ip.hpp"

class ForwardingProcess : public Process
{
private:
    Node *current_node;
    Interface *ingress_intf;
    Node *l3_nhop;

public:
    ForwardingProcess();

    void init(Node *);
    void exec_step(Network&, const EqClass&) override;
};
