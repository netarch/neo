#include <csignal>
#include <unistd.h>
#include <regex>

#include "policy/policy.hpp"
#include "lib/logger.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/stateful-reachability.hpp"
#include "policy/waypoint.hpp"

Policy::Policy(const std::shared_ptr<cpptoml::table>& config,
               const Network& net)
    : prerequisite(nullptr)
{
    static int next_id = 1;
    id = next_id++;

    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");
    auto start_regex = config->get_as<std::string>("start_node");

    if (!pkt_dst_str) {
        Logger::get().err("Missing packet destination");
    }
    if (!start_regex) {
        Logger::get().err("Missing start node");
    }

    std::string dst_str = *pkt_dst_str;
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    pkt_dst = IPRange<IPv4Address>(dst_str);

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
    }

    // NOTE: fixed port numbers for now
    src_port = 1234;
    dst_port = 80;
}

Policy::Policy(const IPRange<IPv4Address>& pkt_dst,
               const std::vector<Node *> start_nodes)
    : id(0), pkt_dst(pkt_dst), start_nodes(start_nodes), prerequisite(nullptr)
{
}

int Policy::get_id() const
{
    return id;
}

const std::vector<Node *>& Policy::get_start_nodes(State *state) const
{
    if (prerequisite && state->comm == 0) {
        return prerequisite->start_nodes;
    } else {
        return start_nodes;
    }
}

uint16_t Policy::get_src_port(State *state) const
{
    bool is_reply = state->comm_state[state->comm].pkt_state | PS_REP;

    if (prerequisite && state->comm == 0) {
        if (is_reply) {
            return prerequisite->dst_port;
        } else {
            return prerequisite->src_port;
        }
    } else {
        if (is_reply) {
            return dst_port;
        } else {
            return src_port;
        }
    }
}

uint16_t Policy::get_dst_port(State *state) const
{
    bool is_reply = state->comm_state[state->comm].pkt_state | PS_REP;

    if (prerequisite && state->comm == 0) {
        if (is_reply) {
            return prerequisite->src_port;
        } else {
            return prerequisite->dst_port;
        }
    } else {
        if (is_reply) {
            return src_port;
        } else {
            return dst_port;
        }
    }
}

Policy *Policy::get_prerequisite() const
{
    return prerequisite;
}

const EqClasses& Policy::get_ecs() const
{
    return ECs;
}

size_t Policy::num_ecs() const
{
    if (prerequisite) {
        return ECs.size() * prerequisite->num_ecs();
    } else {
        return ECs.size();
    }
}

void Policy::compute_ecs(const EqClasses& all_ECs)
{
    ECs.add_mask_range(pkt_dst, all_ECs);
    if (prerequisite) {
        prerequisite->compute_ecs(all_ECs);
    }
}

void Policy::report(State *state) const
{
    if (state->comm_state[state->comm].violated) {
        Logger::get().info("Policy violated!");
        kill(getppid(), SIGUSR1);
    } else {
        Logger::get().info("Policy holds!");
    }
}

/******************************************************************************/

Policies::Policies(const std::shared_ptr<cpptoml::table_array>& configs,
                   const Network& network)
{
    if (configs) {
        for (auto config : *configs) {
            Policy *policy = nullptr;
            auto type = config->get_as<std::string>("type");

            if (!type) {
                Logger::get().err("Missing policy type");
            }

            if (*type == "reachability") {
                policy = new ReachabilityPolicy(config, network);
            } else if (*type == "waypoint") {
                policy = new WaypointPolicy(config, network);
            } else if (*type == "reply-reachability") {
                policy = new ReplyReachabilityPolicy(config, network);
            } else if (*type == "stateful-reachability") {
                policy = new StatefulReachabilityPolicy(config, network);
            } else {
                Logger::get().err("Unknown policy type: " + *type);
            }

            policies.push_back(policy);
        }
    }
    if (policies.size() == 1) {
        Logger::get().info("Loaded 1 policy");
    } else {
        Logger::get().info("Loaded " + std::to_string(policies.size())
                           + " policies");
    }
    compute_ecs(network);
}

Policies::~Policies()
{
    for (Policy *policy : policies) {
        delete policy;
    }
}

void Policies::compute_ecs(const Network& network) const
{
    EqClasses all_ECs;

    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            all_ECs.add_ec(intf.first);
        }
        for (const Route& route : node.second->get_rib()) {
            all_ECs.add_ec(route.get_network());
        }
    }

    for (Policy *policy : policies) {
        policy->compute_ecs(all_ECs);
    }
}

Policies::iterator Policies::begin()
{
    return policies.begin();
}

Policies::const_iterator Policies::begin() const
{
    return policies.begin();
}

Policies::iterator Policies::end()
{
    return policies.end();
}

Policies::const_iterator Policies::end() const
{
    return policies.end();
}

Policies::reverse_iterator Policies::rbegin()
{
    return policies.rbegin();
}

Policies::const_reverse_iterator Policies::rbegin() const
{
    return policies.rbegin();
}

Policies::reverse_iterator Policies::rend()
{
    return policies.rend();
}

Policies::const_reverse_iterator Policies::rend() const
{
    return policies.rend();
}
