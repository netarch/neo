#pragma once

#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "lib/ip.hpp"

/*
 * For all possible packets starting from start_node, with source and
 * destination addresses within pkt_src and pkt_dst, respectively, the packet
 * will eventually be accepted by final_node when reachable is true. Otherwise,
 * if reachable is false, the packet will either be dropped by final_node, or
 * never reach final_node.
 */
class ReachabilityPolicy : public Policy
{
private:
    IPRange<IPv4Address> pkt_src;
    IPRange<IPv4Address> pkt_dst;
    std::shared_ptr<Node> start_node;
    std::shared_ptr<Node> final_node;
    bool reachable;

    //std::shared_ptr<Node> pkt_location; --> forwarding process

public:
    ReachabilityPolicy() = default;
    ReachabilityPolicy(const ReachabilityPolicy&) = default;

    //virtual void load_config(const std::shared_ptr<cpptoml::table>&) override;
    //virtual bool check_violation() override;
};
