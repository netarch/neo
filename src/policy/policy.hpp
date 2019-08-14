#pragma once

#include <string>
#include <vector>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"
#include "eqclasses.hpp"
#include "network.hpp"
#include "process/forwarding.hpp"
#include "pan.h"

class Policy
{
protected:
    IPRange<IPv4Address>    pkt_src;
    IPRange<IPv4Address>    pkt_dst;
    EqClasses               ECs;        // ECs to be verified
    bool                    violated;

public:
    Policy(const std::shared_ptr<cpptoml::table>&);
    virtual ~Policy() = default;

    void compute_ecs(const Network&);
    const IPRange<IPv4Address>& get_pkt_src() const;
    const IPRange<IPv4Address>& get_pkt_dst() const;
    const EqClasses& get_ecs() const;

    virtual std::string to_string() const;
    virtual std::string get_type() const;
    virtual void procs_init(State *, ForwardingProcess&) const;
    virtual void check_violation(State *);
    virtual void report(State *) const;
};
