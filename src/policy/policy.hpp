#pragma once

#include <string>
#include <list>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"
#include "eqclasses.hpp"
#include "network.hpp"
class ForwardingProcess;
#include "process/forwarding.hpp"
#include "pan.h"

class Policy
{
protected:
    int                     id;
    IPRange<IPv4Address>    pkt_dst;
    std::vector<Node *>     start_nodes;
    uint16_t                src_port, dst_port;
    EqClasses               ECs;
    Policy                  *prerequisite;
    // assuming only 2 correlated ECs for now
    // may be changed to a list of prerequisite Policies in the future
    // or defined recursively

public:
    Policy(const std::shared_ptr<cpptoml::table>&, const Network&);
    Policy(const IPRange<IPv4Address>& pkt_dst,
           const std::vector<Node *> start_nodes);
    virtual ~Policy() = default;

    int get_id() const;
    const std::vector<Node *>& get_start_nodes(State *) const;
    uint16_t get_src_port(State *) const;
    uint16_t get_dst_port(State *) const;
    Policy *get_prerequisite() const;

    virtual const EqClasses& get_ecs() const;
    virtual size_t num_ecs() const;
    virtual void compute_ecs(const EqClasses&);
    virtual void report(State *) const;

    virtual std::string to_string() const = 0;
    virtual void init(State *) const = 0;
    virtual void check_violation(State *) = 0;
};

class Policies
{
private:
    std::list<Policy *> policies;

    void compute_ecs(const Network&) const;

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

    Policies(const std::shared_ptr<cpptoml::table_array>&, const Network&);
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
