#pragma once

#include <vector>
#include <string>
#include <list>

#include "comm.hpp"
#include "lib/ip.hpp"
#include "node.hpp"
#include "network.hpp"
#include "eqclasses.hpp"
#include "eqclass.hpp"
struct State;

#define POL_NULL        0
#define POL_INIT_POL    1
#define POL_INIT_FWD    2
#define POL_RESET_FWD   4

class Policy
{
protected:
    int id;

    /* Simultaneously modelled communications. */
    std::vector<Communication> comms;

    /*
     * If there is any correlated policy, all of them would be
     * single-communication policies, and the number of communications of the
     * main policy (comms) would be zero.
     */
    std::vector<Policy *> correlated_policies;

protected:
    friend class Config;
    Policy(bool correlated = false);

public:
    virtual ~Policy();

    int get_id() const;

    int get_protocol(State *) const;
    const std::vector<Node *>& get_start_nodes(State *) const;
    uint16_t get_src_port(State *) const;
    uint16_t get_dst_port(State *) const;

    void compute_ecs(const EqClasses&, const EqClasses&);
    void add_ec(State *, const IPv4Address&);
    EqClass *find_ec(State *, const IPv4Address&) const;
    size_t num_ecs() const;
    size_t num_comms() const;       // total number of communications
    size_t num_simul_comms() const; // number of simultaneous communications
    // TODO: change the name from simultaneous to active?

    bool set_initial_ec();
    EqClass *get_initial_ec(State *) const;

    void report(State *) const;

    virtual std::string to_string() const = 0;
    virtual void init(State *) = 0;
    virtual int check_violation(State *) = 0;
};

class Policies
{
private:
    std::list<Policy *> policies;

private:
    friend class Config;

public:
    typedef std::list<Policy *>::size_type size_type;
    typedef std::list<Policy *>::iterator iterator;
    typedef std::list<Policy *>::const_iterator const_iterator;
    typedef std::list<Policy *>::reverse_iterator reverse_iterator;
    typedef std::list<Policy *>::const_reverse_iterator const_reverse_iterator;

    Policies() = default;
    Policies(const Policies&) = delete;
    Policies(Policies&&) = delete;
    Policies& operator=(const Policies&) = delete;
    Policies& operator=(Policies&&) = default;

    ~Policies();

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};
