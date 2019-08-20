#pragma once

#include <string>
#include <list>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"
#include "eqclasses.hpp"
#include "network.hpp"
#include "process/forwarding.hpp"
#include "pan.h"

class Policy
{
protected:
    int                     id;
    IPRange<IPv4Address>    pkt_dst;
    EqClasses               ECs;        // ECs to be verified

public:
    Policy(const std::shared_ptr<cpptoml::table>&);
    virtual ~Policy() = default;

    int get_id() const;
    const IPRange<IPv4Address>& get_pkt_src() const;
    const IPRange<IPv4Address>& get_pkt_dst() const;
    const EqClasses& get_ecs() const;

    virtual const EqClasses& get_pre_ecs() const;
    virtual size_t num_ecs() const;
    virtual void compute_ecs(const EqClasses&);
    virtual std::string to_string() const = 0;
    virtual std::string get_type() const = 0;
    virtual void init(State *) = 0;
    virtual void config_procs(State *, ForwardingProcess&) const = 0;
    virtual void check_violation(State *) = 0;
    virtual void report(State *) const;
};

class Policies
{
private:
    std::list<Policy *> policies;

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

    void compute_ecs(const Network&) const;

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};
