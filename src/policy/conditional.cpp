#include "policy/conditional.hpp"

#include "process/forwarding.hpp"
#include "model.h"

std::string ConditionalPolicy::to_string() const
{
    return std::string();
    //std::string ret = "stateful-reachability [";
    //for (Node *node : start_nodes) {
    //    ret += " " + node->to_string();
    //}
    //ret += " ] -";
    //if (reachable) {
    //    ret += "-";
    //} else {
    //    ret += "X";
    //}
    //ret += "-> [";
    //for (Node *node : final_nodes) {
    //    ret += " " + node->to_string();
    //}
    //ret += " ], with prerequisite: " + prerequisite->to_string();
    //return ret;
}

void ConditionalPolicy::init(State *state)
{
    correlated_policies[state->comm]->init(state);
}

int ConditionalPolicy::check_violation(State *state __attribute__((unused)))
{
    // TODO
    return POL_NULL;
}
