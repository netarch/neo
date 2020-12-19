#include "policy/one-request.hpp"

#include <regex>

//#include "process/forwarding.hpp"
#include "model.h"

OneRequestPolicy::OneRequestPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    auto server_regex = config->get_as<std::string>("server_node");
    auto comm_cfg = config->get_table("communication");

    if (!server_regex) {
        Logger::get().err("Missing server node");
    }
    if (!comm_cfg) {
        Logger::get().err("Missing communication");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*server_regex))) {
            server_nodes.insert(node.second);
        }
    }

    Communication comm(comm_cfg, net);
    comms.push_back(std::move(comm));
}

std::string OneRequestPolicy::to_string() const
{
    std::string ret = "one-request among:\n";
    for (const Communication& comm : comms) {
        ret += comm.start_nodes_str() + " ---> [";
        for (Node *node : server_nodes) {
            ret += " " + node->to_string();
        }
        ret += " ]\n";
    }
    ret.pop_back();
    return ret;
}

void OneRequestPolicy::init(State *state)
{
    state->violated = true;
}

// TODO
int OneRequestPolicy::check_violation(State *state __attribute__((unused)))
{
    //bool reached;
    //int mode = state->comm_state[state->comm].fwd_mode;
    //uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    //if (mode == fwd_mode::ACCEPTED &&
    //        (pkt_state == PS_HTTP_REQ || pkt_state == PS_ICMP_ECHO_REQ)) {
    //    Node *rx_node;
    //    memcpy(&rx_node, state->comm_state[state->comm].rx_node,
    //           sizeof(Node *));
    //    reached = (final_nodes.count(rx_node) > 0);
    //} else if (mode == fwd_mode::DROPPED) {
    //    reached = false;
    //} else {
    //    /*
    //     * If the packet hasn't been accepted or dropped, there is nothing to
    //     * check.
    //     */
    //    return POL_NULL;
    //}

    //state->violated = (reachable != reached);
    //state->choice_count = 0;

    return POL_NULL;
}
