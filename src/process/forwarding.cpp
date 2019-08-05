#include "process/forwarding.hpp"

ForwardingProcess::ForwardingProcess(): current_node(nullptr),
    ingress_intf(nullptr), l3_nhop(nullptr)
{
}

void ForwardingProcess::init(Node *start_node)
{
    current_node = start_node;
}

void ForwardingProcess::exec_step(Network& network __attribute__((unused)),
                                  const EqClass& ec __attribute__((unused)))
{
    // TODO
}
