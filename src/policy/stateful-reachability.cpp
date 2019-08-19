#include <regex>

#include "policy/stateful-reachability.hpp"
#include "policy/reachability.hpp"
#include "policy/waypoint.hpp"

StatefulReachabilityPolicy::StatefulReachabilityPolicy(
    const std::shared_ptr<cpptoml::table>& config,
    const Network& net)
    : Policy(config)
{
    auto start_regex = config->get_as<std::string>("start_node");
    auto final_regex = config->get_as<std::string>("final_node");
    auto reachability = config->get_as<bool>("reachable");
    auto prereq = config->get_table("prerequisite");

    if (!start_regex) {
        Logger::get_instance().err("Missing start node");
    }
    if (!final_regex) {
        Logger::get_instance().err("Missing final node");
    }
    if (!reachability) {
        Logger::get_instance().err("Missing reachability");
    }
    if (!prereq) {
        Logger::get_instance().err("Missing prerequisite policy");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            final_nodes.push_back(node.second);
        }
    }

    reachable = *reachability;

    auto type = prereq->get_as<std::string>("type");

    if (!type) {
        Logger::get_instance().err("Missing policy type");
    }

    if (*type == "reachability") {
        prerequisite = new ReachabilityPolicy(prereq, net);
    } else if (*type == "waypoint") {
        prerequisite = new WaypointPolicy(prereq, net);
    } else {
        Logger::get_instance().err("Unsupported prerequisite policy type: "
                                   + *type);
    }
}

StatefulReachabilityPolicy::~StatefulReachabilityPolicy()
{
    delete prerequisite;
}

const EqClasses& StatefulReachabilityPolicy::get_pre_ecs() const
{
    return prerequisite->get_ecs();
}

size_t StatefulReachabilityPolicy::num_ecs() const
{
    return ECs.size() * prerequisite->get_ecs().size();
}

void StatefulReachabilityPolicy::compute_ecs(const EqClasses& all_ECs)
{
    Policy::compute_ecs(all_ECs);
    prerequisite->compute_ecs(all_ECs);
}

std::string StatefulReachabilityPolicy::to_string() const
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

std::string StatefulReachabilityPolicy::get_type() const
{
    return "stateful-reachability policy";
}

void StatefulReachabilityPolicy::init(State *state)
{
    if (state->itr_ec == 0) {
        prerequisite->init(state);
        return;
    }

    state->network_state[state->itr_ec].violated = false;
}

void StatefulReachabilityPolicy::config_procs(State *state,
        ForwardingProcess& fwd) const
{
    if (state->itr_ec == 0) {
        prerequisite->config_procs(state, fwd);
        return;
    }

    fwd.config(state, start_nodes);
    fwd.enable();
}

void StatefulReachabilityPolicy::check_violation(State *state)
{
    if (state->itr_ec == 0) {
        prerequisite->check_violation(state);
        if (state->choice_count == 0) {
            if (state->network_state[state->itr_ec].violated) {
                // prerequisite policy violated
                ++state->itr_ec;
                state->network_state[state->itr_ec].violated = false;
                state->choice_count = 0;
            } else {
                // prerequisite policy holds
                ++state->itr_ec;
                state->choice_count = 1;
            }
        }
        return;
    }

    bool reached;
    auto& current_fwd_mode = state->network_state[state->itr_ec].fwd_mode;

    if (current_fwd_mode == fwd_mode::ACCEPTED) {
        Node *final_node;
        memcpy(&final_node, state->network_state[state->itr_ec].pkt_location,
               sizeof(Node *));
        reached = (std::find(final_nodes.begin(), final_nodes.end(), final_node)
                   != final_nodes.end());
    } else if (current_fwd_mode == fwd_mode::DROPPED) {
        reached = false;
    } else {
        /*
         * If the packet hasn't been accepted or dropped, there is nothing to
         * check.
         */
        return;
    }

    state->network_state[state->itr_ec].violated = (reachable != reached);
    state->choice_count = 0;
}
