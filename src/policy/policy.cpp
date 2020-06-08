#include "policy/policy.hpp"

#include <csignal>
#include <iostream>
#include <unistd.h>

#include "lib/logger.hpp"
#include "packet.hpp"
#include "policy/conditional.hpp"
#include "policy/consistency.hpp"
#include "policy/loadbalance.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "process/openflow.hpp"
#include "model.h"

Policy::Policy(bool correlated)
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

        if (*type == "loadbalance") {
            policy = new LoadBalancePolicy(config, network, true);
        } else if (*type == "reachability") {
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
        for (Communication& comm : comms) {
            comm.compute_ecs(all_ECs, owned_ECs);
        }
    } else {
        for (Policy *p : correlated_policies) {
            p->compute_ecs(all_ECs, owned_ECs);
        }
    }
}

int Policy::get_id() const
{
    return id;
}

int Policy::get_protocol(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_protocol();
    } else {
        return correlated_policies[state->comm]->comms[0].get_protocol();
    }
}

const std::vector<Node *>& Policy::get_start_nodes(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_start_nodes();
    } else {
        return correlated_policies[state->comm]->comms[0].get_start_nodes();
    }
}

uint16_t Policy::get_src_port(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_src_port(state);
    } else {
        return correlated_policies[state->comm]->comms[0].get_src_port(state);
    }
}

uint16_t Policy::get_dst_port(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_dst_port(state);
    } else {
        return correlated_policies[state->comm]->comms[0].get_dst_port(state);
    }
}

void Policy::add_ec(State *state, const IPv4Address& addr)
{
    if (correlated_policies.empty()) {
        comms[state->comm].add_ec(addr);
    } else {
        correlated_policies[state->comm]->comms[0].add_ec(addr);
    }
}

EqClass *Policy::find_ec(State *state, const IPv4Address& addr) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].find_ec(addr);
    } else {
        return correlated_policies[state->comm]->comms[0].find_ec(addr);
    }
}

size_t Policy::num_ecs() const
{
    if (correlated_policies.empty()) {
        size_t num = 1;
        for (const Communication& comm : comms) {
            num *= comm.num_ecs();
        }
        return num;
    } else {
        size_t num = 1;
        for (Policy *p : correlated_policies) {
            num *= p->comms[0].num_ecs();
        }
        return num;
    }
}

size_t Policy::num_comms() const
{
    if (correlated_policies.empty()) {
        size_t num = 1;
        for (const Communication& comm : comms) {
            num *= comm.num_comms();
        }
        return num;
    } else {
        size_t total_num_start_nodes = 0;
        for (Policy *p : correlated_policies) {
            total_num_start_nodes += p->comms[0].num_start_nodes();
        }
        return num_ecs() * total_num_start_nodes;
    }
}

bool Policy::set_initial_ec()
{
    if (correlated_policies.empty()) {
        bool first_run = false;
        if (!comms[0].get_initial_ec()) {   // first run
            first_run = true;
        }
        for (Communication& comm : comms) {
            if (comm.set_initial_ec()) {
                return true;
            }
            // return false for carrying the next "tick"
        }
        return first_run;
    } else {
        bool first_run = false;
        if (!correlated_policies[0]->comms[0].get_initial_ec()) {
            first_run = true;
        }
        for (Policy *p : correlated_policies) {
            if (p->comms[0].set_initial_ec()) {
                return true;
            }
            // return false for carrying the next "tick"
        }
        return first_run;
    }
}

EqClass *Policy::get_initial_ec(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_initial_ec();
    } else {
        return correlated_policies[state->comm]->comms[0].get_initial_ec();
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

            if (*type == "loadbalance") {
                policy = new LoadBalancePolicy(config, network);
            } else if (*type == "reachability") {
                policy = new ReachabilityPolicy(config, network);
            } else if (*type == "reply-reachability") {
                policy = new ReplyReachabilityPolicy(config, network);
            } else if (*type == "waypoint") {
                policy = new WaypointPolicy(config, network);
            } else if (*type == "conditional") {
                policy = new ConditionalPolicy(config, network);
            } else if (*type == "consistency") {
                policy = new ConsistencyPolicy(config, network);
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

    for (const auto& update_entry : Openflow::get_updates()) {
        for (const auto& update_route : update_entry.second) {
            all_ECs.add_ec(update_route.get_network());
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
