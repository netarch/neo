#pragma once

#include <memory>
#include <string>
#include <cpptoml/cpptoml.hpp>

#include "lib/ip.hpp"
#include "eqclasses.hpp"
#include "network.hpp"
#include "process/forwarding.hpp"
#include "pan.h"

class Policy
{
protected:
    IPRange<IPv4Address> pkt_src;
    IPRange<IPv4Address> pkt_dst;
    EqClasses            ECs;       // ECs to be verified

public:
    Policy(const std::shared_ptr<cpptoml::table>&);
    virtual ~Policy() = default;

    const IPRange<IPv4Address>& get_pkt_src() const;
    const IPRange<IPv4Address>& get_pkt_dst() const;
    EqClasses& get_ecs();
    const EqClasses& get_ecs() const;

    void compute_ecs(const Network&);

    virtual std::string to_string() const;
    virtual std::string get_type() const;
    virtual void config_procs(State *, ForwardingProcess&) const;
    //virtual bool check_violation(const Network&, const ForwardingProcess&);
};
