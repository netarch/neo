#include "policy/policy.hpp"

#include <csignal>
#include <unistd.h>
#include <cctype>
#include <regex>
#include <algorithm>

#include "lib/logger.hpp"
#include "packet.hpp"
#include "policy/conditional.hpp"
#include "policy/consistency.hpp"
#include "policy/loadbalance.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "model.h"

Policy::Policy(bool correlated)
    : initial_ec(nullptr), comm_tx(nullptr), comm_rx(nullptr)
{
    static int next_id = 1;
    if (correlated) {
        id = 0;
    } else {
        id = next_id++;
    }
}

Policy::~Policy()
{
    for (Policy *p : correlated_policies) {
        delete p;
    }
}

void Policy::parse_protocol(const std::shared_ptr<cpptoml::table>& config)
{
    auto proto_str = config->get_as<std::string>("protocol");

    if (!proto_str) {
        Logger::get().err("Missing protocol");
    }

    std::string proto_s = *proto_str;
    std::transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
    [](unsigned char c) {
        return std::tolower(c);
    });
    if (proto_s == "http") {
        protocol = proto::PR_HTTP;
    } else if (proto_s == "icmp-echo") {
        protocol = proto::PR_ICMP_ECHO;
    } else {
        Logger::get().err("Unknown protocol: " + *proto_str);
    }
}

void Policy::parse_pkt_dst(const std::shared_ptr<cpptoml::table>& config)
{
    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");

    if (!pkt_dst_str) {
        Logger::get().err("Missing packet destination");
    }

    std::string dst_str = *pkt_dst_str;
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    pkt_dst = IPRange<IPv4Address>(dst_str);
}

void Policy::parse_owned_dst_only(const std::shared_ptr<cpptoml::table>& config)
{
    auto owned_only = config->get_as<bool>("owned_dst_only");

    if (owned_only) {
        owned_dst_only = *owned_only;
    } else {
        owned_dst_only = true;
    }
}

void Policy::parse_start_node(const std::shared_ptr<cpptoml::table>& config,
                              const Network& net)
{
    auto start_regex = config->get_as<std::string>("start_node");

    if (!start_regex) {
        Logger::get().err("Missing start node");
    }

    const std::map<std::string, Node *>& nodes = net.get_nodes();
    for (const auto& node : nodes) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            start_nodes.push_back(node.second);
        }
    }
}

void Policy::parse_tcp_ports(
    const std::shared_ptr<cpptoml::table>& config __attribute__((unused)))
{
    // NOTE: fixed port numbers for now
    tx_port = 1234;
    rx_port = 80;
}

void Policy::parse_correlated_policies(
    const std::shared_ptr<cpptoml::table>& config, const Network& network)
{
    auto policies_config = config->get_table_array("correlated_policies");

    if (!policies_config) {
        Logger::get().err("Missing correlated policies");
    }

    for (auto config : *policies_config) {
        Policy *policy = nullptr;
        auto type = config->get_as<std::string>("type");

        if (!type) {
            Logger::get().err("Missing policy type");
        }

        if (*type == "reachability") {
            policy = new ReachabilityPolicy(config, network, true);
        } else if (*type == "reply-reachability") {
            policy = new ReplyReachabilityPolicy(config, network, true);
        } else if (*type == "waypoint") {
            policy = new WaypointPolicy(config, network, true);
        } else {
            Logger::get().err("Unsupported policy type: " + *type);
        }

        correlated_policies.push_back(policy);
    }
}

void Policy::compute_ecs(const EqClasses& all_ECs, const EqClasses& owned_ECs)
{
    if (correlated_policies.empty()) {
        if (owned_dst_only) {
            ECs.add_mask_range(pkt_dst, owned_ECs);
        } else {
            ECs.add_mask_range(pkt_dst, all_ECs);
        }
    } else {
        for (Policy *p : correlated_policies) {
            if (p->owned_dst_only) {
                p->ECs.add_mask_range(p->pkt_dst, owned_ECs);
            } else {
                p->ECs.add_mask_range(p->pkt_dst, all_ECs);
            }
        }
    }
}

int Policy::get_id() const
{
    return id;
}

int Policy::get_protocol(State *state) const
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->protocol;
    } else {
        return protocol;
    }
}

const std::vector<Node *>& Policy::get_start_nodes(State *state) const
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->start_nodes;
    } else {
        return start_nodes;
    }
}

uint16_t Policy::get_src_port(State *state) const
{
    bool is_reply = PS_IS_REPLY(state->comm_state[state->comm].pkt_state);

    if (!correlated_policies.empty()) {
        if (is_reply) {
            return correlated_policies[state->comm]->rx_port;
        } else {
            return correlated_policies[state->comm]->tx_port;
        }
    } else {
        if (is_reply) {
            return rx_port;
        } else {
            return tx_port;
        }
    }
}

uint16_t Policy::get_dst_port(State *state) const
{
    bool is_reply = PS_IS_REPLY(state->comm_state[state->comm].pkt_state);

    if (!correlated_policies.empty()) {
        if (is_reply) {
            return correlated_policies[state->comm]->tx_port;
        } else {
            return correlated_policies[state->comm]->rx_port;
        }
    } else {
        if (is_reply) {
            return tx_port;
        } else {
            return rx_port;
        }
    }
}

void Policy::add_ec(State *state, const IPv4Address& addr)
{
    if (!correlated_policies.empty()) {
        correlated_policies[state->comm]->ECs.add_ec(addr);
    } else {
        ECs.add_ec(addr);
    }
}

EqClass *Policy::find_ec(State *state, const IPv4Address& addr)
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->ECs.find_ec(addr);
    } else {
        return ECs.find_ec(addr);
    }
}

const EqClasses& Policy::get_ecs(State *state) const
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->ECs;
    } else {
        return ECs;
    }
}

size_t Policy::num_ecs() const
{
    if (!correlated_policies.empty()) {
        size_t num = 1;
        for (Policy *p : correlated_policies) {
            num *= p->ECs.size();
        }
        return num;
    } else {
        return ECs.size();
    }
}

size_t Policy::num_comms() const
{
    if (!correlated_policies.empty()) {
        // NOTE: we assume that the multiple communications are modelled
        // sequencially (i.e. one starts after another ends), so the number of
        // communications is the number of all EC combinations multiplied by the
        // number of total start nodes of all correlated policies.
        size_t num_start_nodes = 0;
        for (Policy *p : correlated_policies) {
            num_start_nodes += p->start_nodes.size();
        }
        return num_ecs() * num_start_nodes;
    } else {
        return num_ecs() * start_nodes.size();
    }
}

bool Policy::set_initial_ec()
{
    if (correlated_policies.empty()) {
        if (!initial_ec) {  // first run
            ECs_itr = ECs.begin();
            initial_ec = *ECs_itr;
            return true;
        } else {    // subsequent calls
            if (++ECs_itr != ECs.end()) {
                initial_ec = *ECs_itr;
                return true;
            }
            return false;
        }
    } else {
        if (!correlated_policies[0]->initial_ec) {  // first run
            for (Policy *p : correlated_policies) {
                p->ECs_itr = p->ECs.begin();
                p->initial_ec = *p->ECs_itr;
            }
            return true;
        } else {    // subsequent calls
            for (Policy *p : correlated_policies) {
                if (++p->ECs_itr != p->ECs.end()) {
                    p->initial_ec = *p->ECs_itr;
                    return true;
                }
                p->ECs_itr = p->ECs.begin();
                p->initial_ec = *p->ECs_itr;
            }
            return false;
        }
    }
}

EqClass *Policy::get_initial_ec(State *state) const
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->initial_ec;
    } else {
        return initial_ec;
    }
}

void Policy::set_comm_tx(State *state, Node *node)
{
    if (!correlated_policies.empty()) {
        correlated_policies[state->comm]->comm_tx = node;
    } else {
        comm_tx = node;
    }
}

void Policy::set_comm_rx(State *state, Node *node)
{
    if (!correlated_policies.empty()) {
        correlated_policies[state->comm]->comm_rx = node;
    } else {
        comm_rx = node;
    }
}

Node *Policy::get_comm_tx(State *state)
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->comm_tx;
    } else {
        return comm_tx;
    }
}

Node *Policy::get_comm_rx(State *state)
{
    if (!correlated_policies.empty()) {
        return correlated_policies[state->comm]->comm_rx;
    } else {
        return comm_rx;
    }
}

void Policy::report(State *state) const
{
    if (state->violated) {
        Logger::get().info("*** Policy violated! ***");
        kill(getppid(), SIGUSR1);
    } else {
        Logger::get().info("*** Policy holds! ***");
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
            } else if (*type == "reply-reachability") {
                policy = new ReplyReachabilityPolicy(config, network);
            } else if (*type == "waypoint") {
                policy = new WaypointPolicy(config, network);
            } else if (*type == "conditional") {
                policy = new ConditionalPolicy(config, network);
            } else if (*type == "consistency") {
                policy = new ConsistencyPolicy(config, network);
            } else if (*type == "loadbalance") {
                policy = new LoadBalancePolicy(config, network);
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
    EqClasses all_ECs, owned_ECs;

    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            all_ECs.add_ec(intf.first);
            owned_ECs.add_ec(intf.first);
        }
        for (const Route& route : node.second->get_rib()) {
            all_ECs.add_ec(route.get_network());
        }
    }

    for (Policy *policy : policies) {
        policy->compute_ecs(all_ECs, owned_ECs);
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
