#include "policy/policy.hpp"

#include <cassert>
#include <csignal>
#include <unistd.h>

#include "lib/logger.hpp"
#include "policy/conditional.hpp"
#include "policy/consistency.hpp"
#include "policy/loadbalance.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "model-access.hpp"
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

size_t Policy::num_conn_ecs() const
{
    if (correlated_policies.empty()) {
        return conn_matrix.num_conns();
    } else {
        size_t num = 1;
        for (Policy *p : correlated_policies) {
            num *= p->conn_matrix.num_conns();
        }
        return num;
    }
}

void Policy::compute_conn_matrix()
{
    // update policy-wide ECs with policy-aware ranges
    if (correlated_policies.empty()) {
        for (const ConnSpec& conn_spec : conn_specs) {
            conn_spec.update_policy_ecs();
        }
    } else {
        for (Policy *p : correlated_policies) {
            p->conn_specs[0].update_policy_ecs();
        }
    }

    // gather conn_specs' connections
    if (correlated_policies.empty()) {
        conn_matrix.clear();
        for (const ConnSpec& conn_spec : conn_specs) {
            conn_matrix.add(conn_spec.compute_connections());
        }
    } else {
        for (Policy *p : correlated_policies) {
            p->conn_matrix.clear();
            p->conn_matrix.add(p->conn_specs[0].compute_connections());
        }
    }
}

bool Policy::set_conns()
{
    if (correlated_policies.empty()) {
        conns = conn_matrix.get_next_conns();
        if (!conns.empty()) {
            return true;
        }
        return false;
    } else {
        for (Policy *p : correlated_policies) {
            if (p->set_conns()) {
                assert(p->conns.size() == 1);
                return true;
            } // return false for carrying the next "tick"
            p->conn_matrix.reset();
        }
        return false;
    }
}

const std::vector<Connection>& Policy::get_conns() const
{
    return conns;
}

std::string Policy::conns_str() const
{
    std::string ret;
    for (const Connection& conn : conns) {
        ret += conn.to_string() + "\n";
    }
    ret.pop_back();
    return ret;
}

void Policy::report(State *state) const
{
    if (get_violated(state)) {
        Logger::info("*** Policy violated! ***");
        kill(getppid(), SIGUSR1);
    } else {
        Logger::info("*** Policy holds! ***");
    }
}

void Policy::init(State *state, const Network *network)
{
    if (correlated_policies.empty()) {
        // per-connection states
        for (size_t i = 0; i < conns.size(); ++i) {
            conns[i].init(state, i, *network);
        }
        set_conn(state, 0);
        set_num_conns(state, conns.size());
        print_conn_states(state);
    } else {
        correlated_policies[get_correlated_policy_idx(state)]->init(state, network);
    }
}

// reinit should only be overwritten by policies with correlated sub-policies
void Policy::reinit(State *state __attribute__((unused)),
                    const Network *network __attribute__((unused)))
{
    Logger::error("This shouldn't be called");
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
