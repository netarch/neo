#include "policy/conditional.hpp"

#include <regex>

#include "policy/reachability.hpp"
#include "policy/waypoint.hpp"
#include "process/forwarding.hpp"
#include "model.h"

ConditionalPolicy::ConditionalPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy()
{
    auto final_regex = config->get_as<std::string>("final_node");
    auto reachability = config->get_as<bool>("reachable");
    auto prereq = config->get_table("prerequisite");

    if (!final_regex) {
        Logger::get().err("Missing final node");
    }
    if (!reachability) {
        Logger::get().err("Missing reachability");
    }
    if (!prereq) {
        Logger::get().err("Missing prerequisite policy");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.insert(node.second);
        }
    }

    reachable = *reachability;

    auto type = prereq->get_as<std::string>("type");

    if (!type) {
        Logger::get().err("Missing policy type");
    }

    if (*type == "reachability") {
        prerequisite = new ReachabilityPolicy(prereq, net);
    } else if (*type == "waypoint") {
        prerequisite = new WaypointPolicy(prereq, net);
    } else {
        Logger::get().err("Unsupported prerequisite policy type: " + *type);
    }
}

std::string ConditionalPolicy::to_string() const
{
    std::string ret = "stateful-reachability [";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ] -";
    if (reachable) {
        ret += "-";
    } else {
        ret += "X";
    }
    ret += "-> [";
    for (Node *node : final_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ], with prerequisite: " + prerequisite->to_string();
    return ret;
}

void ConditionalPolicy::init(State *state) const
{
    if (state->comm == 0) {
        prerequisite->init(state);
        return;
    }

    state->violated = false;
}

void ConditionalPolicy::check_violation(State *state)
{
    if (state->comm == 0) {
        prerequisite->check_violation(state);
        if (state->choice_count == 0) {
            if (state->violated) {
                // prerequisite policy violated
                ++state->comm;
                state->violated = false;
                state->choice_count = 0;
            } else {
                // prerequisite policy holds
                ++state->comm;
                state->choice_count = 1;
            }
        }
        return;
    }

    bool reached;
    int mode = state->comm_state[state->comm].fwd_mode;

    if (mode == fwd_mode::ACCEPTED) {
        Node *final_node;
        memcpy(&final_node, state->comm_state[state->comm].pkt_location,
               sizeof(Node *));
        reached = (final_nodes.count(final_node) > 0);
    } else if (mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        /*
         * If the packet hasn't been accepted or dropped, there is nothing to
         * check.
         */
        return;
    }

    state->violated = (reachable != reached);
    state->choice_count = 0;
}
