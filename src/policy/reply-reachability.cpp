#include "policy/reply-reachability.hpp"

#include <regex>

#include "process/forwarding.hpp"
#include "model.h"

ReplyReachabilityPolicy::ReplyReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config, const Network& net,
    bool correlated)
    : Policy(correlated)
{
    auto query_regex = config->get_as<std::string>("query_node");
    auto reachability = config->get_as<bool>("reachable");
    auto comm_cfg = config->get_table("communication");

    if (!query_regex) {
        Logger::get().err("Missing query node");
    }
    if (!reachability) {
        Logger::get().err("Missing reachability");
    }
    if (!comm_cfg) {
        Logger::get().err("Missing communication");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*query_regex))) {
            query_nodes.insert(node.second);
        }
    }

    reachable = *reachability;

    Communication comm(comm_cfg, net);
    comms.push_back(std::move(comm));
}

std::string ReplyReachabilityPolicy::to_string() const
{
    std::string ret = "reply-reachability " + comms[0].start_nodes_str();
    ret += " -> [";
    for (Node *node : query_nodes) {
        ret += " " + node->to_string();
    }
    if (reachable) {
        ret += " ] ---> original sender";
    } else {
        ret += " ] -X-> original sender";
    }
    return ret;
}

void ReplyReachabilityPolicy::init(State *state)
{
    state->violated = false;
}

int ReplyReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    int mode = state->comm_state[state->comm].fwd_mode;
    uint8_t pkt_state = state->comm_state[state->comm].pkt_state;

    if (pkt_state == PS_HTTP_REP || pkt_state == PS_ICMP_ECHO_REP) {
        if (mode == fwd_mode::ACCEPTED) {
            Node *final_node, *tx_node;
            memcpy(&final_node, state->comm_state[state->comm].pkt_location,
                   sizeof(Node *));
            memcpy(&tx_node, state->comm_state[state->comm].tx_node,
                   sizeof(Node *));
            reached = (final_node == tx_node);
        } else if (mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            /*
             * If the reply hasn't been accepted or dropped, there is nothing to
             * check.
             */
            return POL_NULL;
        }
        state->violated = (reachable != reached);
        state->choice_count = 0;
    } else {    // previous phases
        Node *rx_node;
        memcpy(&rx_node, state->comm_state[state->comm].rx_node,
               sizeof(Node *));
        if ((mode == fwd_mode::ACCEPTED && query_nodes.count(rx_node) == 0)
                || mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            /*
             * If the request (or session construction packets) hasn't been
             * accepted or dropped, there is nothing to check.
             */
            return POL_NULL;
        }
        if (!reached) {
            // precondition is false (request not received)
            state->violated = false;
            state->choice_count = 0;
        }
    }

    return POL_NULL;
}
