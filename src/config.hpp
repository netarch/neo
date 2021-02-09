#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <cpptoml.hpp>

#include "lib/ip.hpp"
class Interface;
class Route;
class Node;
class MB_App;
class NetFilter;
class IPVS;
class Squid;
class Middlebox;
class Link;
class Network;
class Communication;
class Policy;
class LoadBalancePolicy;
class ReachabilityPolicy;
class ReplyReachabilityPolicy;
class WaypointPolicy;
class OneRequestPolicy;
class ConditionalPolicy;
class ConsistencyPolicy;
class Policies;
class OpenflowProcess;

class Config
{
private:
    // not thread-safe
    static std::unordered_map<std::string, std::shared_ptr<cpptoml::table>> configs;

    static void parse_interface(Interface *, const std::shared_ptr<cpptoml::table>&);
    static void parse_route(Route *, const std::shared_ptr<cpptoml::table>&);
    static void parse_node(Node *, const std::shared_ptr<cpptoml::table>&);

    static void parse_netfilter(NetFilter *, const std::shared_ptr<cpptoml::table>&);
    static void parse_ipvs(IPVS *, const std::shared_ptr<cpptoml::table>&);
    static void parse_squid(Squid *, const std::shared_ptr<cpptoml::table>&);
    static void parse_middlebox(Middlebox *, const std::shared_ptr<cpptoml::table>&);

    static void parse_link(Link *, const std::shared_ptr<cpptoml::table>&,
                           const std::map<std::string, Node *>&);

    static void parse_communication(Communication *,
                                    const std::shared_ptr<cpptoml::table>&,
                                    const Network&);
    /* single-communication policies */
    static void parse_loadbalancepolicy(LoadBalancePolicy *,
                                        const std::shared_ptr<cpptoml::table>&,
                                        const Network&);
    static void parse_reachabilitypolicy(ReachabilityPolicy *,
                                         const std::shared_ptr<cpptoml::table>&,
                                         const Network&);
    static void parse_replyreachabilitypolicy(ReplyReachabilityPolicy *,
            const std::shared_ptr<cpptoml::table>&,
            const Network&);
    static void parse_waypointpolicy(WaypointPolicy *,
                                     const std::shared_ptr<cpptoml::table>&,
                                     const Network&);
    /* multi-communication policies */
    static void parse_onerequestpolicy(OneRequestPolicy *,
                                       const std::shared_ptr<cpptoml::table>&,
                                       const Network&);
    /* policies with multiple, correlated, single-communication policies */
    static void parse_correlated_policies(Policy *,
                                          const std::shared_ptr<cpptoml::table>&,
                                          const Network&);
    static void parse_conditionalpolicy(ConditionalPolicy *,
                                        const std::shared_ptr<cpptoml::table>&,
                                        const Network&);
    static void parse_consistencypolicy(ConsistencyPolicy *,
                                        const std::shared_ptr<cpptoml::table>&,
                                        const Network&);

    static void parse_openflow_update(Node **,
                                      Route *,
                                      const std::shared_ptr<cpptoml::table>&,
                                      const Network&);

    static void parse_netfilter(const NetFilter *netfilter,
                                std::set<IPNetwork<IPv4Address>>& prefixes,
                                std::set<IPv4Address>& addrs,
                                std::set<uint16_t>& dst_ports);
    static void parse_ipvs(const IPVS *ipvs,
                           std::set<IPNetwork<IPv4Address>>& prefixes,
                           std::set<IPv4Address>& addrs,
                           std::set<uint16_t>& dst_ports);

public:
    static void start_parsing(const std::string& filename);
    static void finish_parsing(const std::string& filename);
    static void parse_network(Network *network, const std::string& filename);
    static void parse_policies(Policies *policies,
                               const std::string& filename,
                               const Network& network);
    static void parse_openflow(OpenflowProcess *openflow,
                               const std::string& filename,
                               const Network& network);
    static void parse_appliance(const MB_App *app,
                                std::set<IPNetwork<IPv4Address>>& prefixes,
                                std::set<IPv4Address>& addrs,
                                std::set<uint16_t>& dst_ports);
};
