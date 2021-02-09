#include "policy/policy.hpp"

#include <csignal>
#include <unistd.h>

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

int Policy::get_id() const
{
    return id;
}

int Policy::get_protocol(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_protocol();
    } else {
        Policy *policy = correlated_policies[state->correlated_policy_idx];
        return policy->comms[0].get_protocol();
    }
}

const std::vector<Node *>& Policy::get_start_nodes(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_start_nodes();
    } else {
        Policy *policy = correlated_policies[state->correlated_policy_idx];
        return policy->comms[0].get_start_nodes();
    }
}

uint16_t Policy::get_src_port(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_src_port();
    } else {
        Policy *policy = correlated_policies[state->correlated_policy_idx];
        return policy->comms[0].get_src_port();
    }
}

uint16_t Policy::get_dst_port(State *state) const
{
    if (correlated_policies.empty()) {
        return comms[state->comm].get_dst_port();
    } else {
        Policy *policy = correlated_policies[state->correlated_policy_idx];
        return policy->comms[0].get_dst_port();
    }
}

void Policy::compute_ecs(const EqClasses& all_ECs, const EqClasses& owned_ECs,
                         const std::set<uint16_t>& dst_ports)
{
    if (correlated_policies.empty()) {
        for (Communication& comm : comms) {
            comm.compute_ecs(all_ECs, owned_ECs, dst_ports);
        }
    } else {
        for (Policy *p : correlated_policies) {
            p->compute_ecs(all_ECs, owned_ECs, dst_ports);
        }
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
        return comms.size();
    } else {
        return correlated_policies.size();
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
        Logger::info("*** Policy violated! ***");
        kill(getppid(), SIGUSR1);
    } else {
        Logger::info("*** Policy holds! ***");
    }
}

/******************************************************************************/

Policies::~Policies()
{
    for (Policy *policy : policies) {
        delete policy;
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
