#include <regex>

#include "policy/reply-reachability.hpp"

ReplyReachabilityPolicy::ReplyReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config, net), queried_node(nullptr)
{
    auto query_regex = config->get_as<std::string>("query_node");
    auto reachability = config->get_as<bool>("reachable");

    if (!query_regex) {
        Logger::get().err("Missing query node");
    }
    if (!reachability) {
        Logger::get().err("Missing reachability");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*query_regex))) {
            query_nodes.insert(node.second);
        }
    }

    reachable = *reachability;
}

std::string ReplyReachabilityPolicy::to_string() const
{
    std::string ret = "reply-reachability [";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ] -> [";
    for (Node *node : query_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ] -";
    if (reachable) {
        ret += "-";
    } else {
        ret += "X";
    }
    ret += "-> original sender";
    return ret;
}

void ReplyReachabilityPolicy::init(State *state) const
{
    state->comm_state[state->comm].violated = false;
}

void ReplyReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    int fwd_mode = state->comm_state[state->comm].fwd_mode;

    if (state->comm == 0) {   // request
        if (fwd_mode == fwd_mode::ACCEPTED) {
            memcpy(&queried_node,
                   state->comm_state[state->comm].pkt_location,
                   sizeof(Node *));
            reached = (query_nodes.count(queried_node) > 0);
        } else if (fwd_mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            /*
             * If the packet hasn't been accepted or dropped, there is nothing
             * to check.
             */
            return;
        }

        if (!reached) {
            // prerequisite policy violated (request not received)
            state->comm_state[state->comm].violated = true;
            ++state->comm;
            state->comm_state[state->comm].violated = false;
            state->choice_count = 0;
        } else {
            // prerequisite policy holds (request received)
            uint32_t req_src;
            memcpy(&req_src, state->comm_state[state->comm].src_ip,
                   sizeof(uint32_t));
            if (req_src == 0) {
                /*
                 * If the request source address hasn't been filled, it means
                 * the queried node is the source node itself, so, WLOG, we
                 * choose the address of its first L3 interface as the reply
                 * destination address.
                 */
                Node *src_node;
                memcpy(&src_node, state->comm_state[state->comm].src_node,
                       sizeof(Node *));
                req_src = src_node->get_intfs_l3().begin()->first.get_value();
            }
            ECs.clear();
            ECs.add_ec(IPv4Address(req_src));
            ++state->comm;
            state->choice_count = 1;
        }
    } else {    // reply
        if (fwd_mode == fwd_mode::ACCEPTED) {
            Node *final_node, *req_src_node;
            memcpy(&final_node,
                   state->comm_state[state->comm].pkt_location,
                   sizeof(Node *));
            memcpy(&req_src_node, state->comm_state[0].src_node,
                   sizeof(Node *));
            reached = (final_node == req_src_node);
        } else if (fwd_mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            /*
             * If the packet hasn't been accepted or dropped, there is nothing
             * to check.
             */
            return;
        }

        state->comm_state[state->comm].violated = (reachable != reached);
        state->choice_count = 0;
    }
}
