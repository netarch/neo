#include <regex>

#include "policy/reply-reachability.hpp"

ReplyReachabilityPolicy::ReplyReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config), queried_node(nullptr)
{
    auto start_regex = config->get_as<std::string>("start_node");
    auto query_regex = config->get_as<std::string>("query_node");
    auto reachability = config->get_as<bool>("reachable");

    if (!start_regex) {
        Logger::get().err("Missing start node");
    }
    if (!query_regex) {
        Logger::get().err("Missing query node");
    }
    if (!reachability) {
        Logger::get().err("Missing reachability");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
        if (std::regex_match(node.first, std::regex(*query_regex))) {
            query_nodes.insert(node.second);
        }
    }

    reachable = *reachability;
}

const EqClasses& ReplyReachabilityPolicy::get_pre_ecs() const
{
    return pre_ECs;
}

const EqClasses& ReplyReachabilityPolicy::get_ecs() const
{
    return ECs;
}

size_t ReplyReachabilityPolicy::num_ecs() const
{
    return pre_ECs.size();
}

void ReplyReachabilityPolicy::compute_ecs(const EqClasses& all_ECs)
{
    pre_ECs.add_mask_range(pkt_dst, all_ECs);
    ECs = EqClasses(nullptr);
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

std::string ReplyReachabilityPolicy::get_type() const
{
    return "reply-reachability";
}

void ReplyReachabilityPolicy::init(State *state)
{
    state->network_state[state->itr_ec].violated = false;
}

void ReplyReachabilityPolicy::config_procs(State *state, const Network& net,
        ForwardingProcess& fwd) const
{
    if (state->itr_ec == 0) {
        fwd.config(state, net, start_nodes);
        fwd.enable();
    } else {
        fwd.config(state, net, std::vector<Node *>(1, queried_node));
        fwd.enable();
    }
}

void ReplyReachabilityPolicy::check_violation(State *state)
{
    bool reached;
    auto& current_fwd_mode = state->network_state[state->itr_ec].fwd_mode;

    if (state->itr_ec == 0) {   // request
        if (current_fwd_mode == fwd_mode::ACCEPTED) {
            memcpy(&queried_node,
                   state->network_state[state->itr_ec].pkt_location,
                   sizeof(Node *));
            reached = (query_nodes.count(queried_node) > 0);
        } else if (current_fwd_mode == fwd_mode::DROPPED) {
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
            state->network_state[state->itr_ec].violated = true;
            ++state->itr_ec;
            state->network_state[state->itr_ec].violated = false;
            state->choice_count = 0;
        } else {
            // prerequisite policy holds (request received)
            uint32_t req_src;
            memcpy(&req_src, state->network_state[state->itr_ec].src_addr,
                   sizeof(uint32_t));
            if (req_src == 0) {
                /*
                 * If the request source address hasn't been filled, it means
                 * the queried node is the source node itself, so, WLOG, we
                 * choose the address of its first L3 interface as the reply
                 * destination address.
                 */
                Node *src_node;
                memcpy(&src_node, state->network_state[state->itr_ec].src_node,
                       sizeof(Node *));
                req_src = src_node->get_intfs_l3().begin()->first.get_value();
            }
            ECs.clear();
            ECs.add_ec(IPv4Address(req_src));
            ++state->itr_ec;
            state->choice_count = 1;
        }
    } else {    // reply
        if (current_fwd_mode == fwd_mode::ACCEPTED) {
            Node *final_node, *req_src_node;
            memcpy(&final_node,
                   state->network_state[state->itr_ec].pkt_location,
                   sizeof(Node *));
            memcpy(&req_src_node, state->network_state[0].src_node,
                   sizeof(Node *));
            reached = (final_node == req_src_node);
        } else if (current_fwd_mode == fwd_mode::DROPPED) {
            reached = false;
        } else {
            /*
             * If the packet hasn't been accepted or dropped, there is nothing
             * to check.
             */
            return;
        }

        state->network_state[state->itr_ec].violated = (reachable != reached);
        state->choice_count = 0;
    }
}
