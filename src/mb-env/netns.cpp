#include "mb-env/netns.hpp"

NetNS::NetNS(const Node *node __attribute__((unused)))
{
    // create a new network namespace
    // create virtual interfaces and bridges
    // update routing table according to node->rib
    // store the information (netns fd?) somehow
    // return to the original network namespace
}

NetNS::~NetNS()
{
}

void NetNS::run(void (*func)(MB_App *), MB_App *app)
{
    // enter the isolated network namespace
    func(app);
    // return to the original network namespace
}
