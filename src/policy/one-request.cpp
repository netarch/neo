#include "policy/one-request.hpp"

#include "node.hpp"
#include "protocols.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

std::string OneRequestPolicy::to_string() const
{
    std::string ret = "One-request:\n"
                      "\ttarget_nodes: [";
    for (Node *node : target_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]\n\t" + conns_str();
    return ret;
}

void OneRequestPolicy::init(State *state, const Network *network) const
{
    Policy::init(state, network);
    set_violated(state, true);
}

int OneRequestPolicy::check_violation(State *state __attribute__((unused)))
{
    // TODO

    return POL_NULL;
}
