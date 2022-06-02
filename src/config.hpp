#pragma once

#include <chrono>
#include <map>
#include <string>
#include <toml++/toml.h>
#include <unordered_map>

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
class ConnSpec;
class Policy;
class ReachabilityPolicy;
class ReplyReachabilityPolicy;
class WaypointPolicy;
class OneRequestPolicy;
class LoadBalancePolicy;
class ConditionalPolicy;
class ConsistencyPolicy;
class Policies;
class OpenflowProcess;

class Config {
private:
    /* config file table (not thread-safe) */
    static std::unordered_map<std::string, std::shared_ptr<toml::table>>
        configs;
    /* packet latency estimate */
    static std::chrono::microseconds latency_avg, latency_mdev;
    static bool got_latency_estimate;

    /* generic network node */
    static void parse_interface(Interface *, const toml::table &);
    static void parse_route(Route *, const toml::table &);
    static void parse_node(Node *, const toml::table &);

    /* middleboxes */
    static void parse_appliance_config(MB_App *, const std::string &);
    static void parse_netfilter(NetFilter *, const toml::table &);
    static void parse_ipvs(IPVS *, const toml::table &);
    static void parse_squid(Squid *, const toml::table &);
    static void parse_middlebox(Middlebox *, const toml::table &, bool);
    static void estimate_latency();

    /* network link */
    static void parse_link(Link *,
                           const toml::table &,
                           const std::map<std::string, Node *> &);

    /* connection */
    static void
    parse_conn_spec(ConnSpec *, const toml::table &, const Network &);
    static void
    parse_connections(Policy *, const toml::table &, const Network &);
    /* single-connection policies */
    static void parse_reachabilitypolicy(ReachabilityPolicy *,
                                         const toml::table &,
                                         const Network &);
    static void parse_replyreachabilitypolicy(ReplyReachabilityPolicy *,
                                              const toml::table &,
                                              const Network &);
    static void parse_waypointpolicy(WaypointPolicy *,
                                     const toml::table &,
                                     const Network &);
    /* multi-connection policies */
    static void parse_onerequestpolicy(OneRequestPolicy *,
                                       const toml::table &,
                                       const Network &);
    static void parse_loadbalancepolicy(LoadBalancePolicy *,
                                        const toml::table &,
                                        const Network &);
    /* policies with multiple correlated sub-policies */
    static void
    parse_correlated_policies(Policy *, const toml::table &, const Network &);
    static void parse_conditionalpolicy(ConditionalPolicy *,
                                        const toml::table &,
                                        const Network &);
    static void parse_consistencypolicy(ConsistencyPolicy *,
                                        const toml::table &,
                                        const Network &);
    /* helper function for parsing policy array to a vector */
    static void parse_policy_array(std::vector<Policy *> &,
                                   bool correlated,
                                   const toml::array &,
                                   const Network &);

    /* openflow updates */
    static void parse_openflow_update(Node **,
                                      Route *,
                                      const toml::table &,
                                      const Network &);

public:
    static void start_parsing(const std::string &filename);
    static void finish_parsing(const std::string &filename);
    static void parse_network(Network *network,
                              const std::string &filename,
                              bool dropmon = false);
    static void parse_policies(Policies *policies,
                               const std::string &filename,
                               const Network &network);
    static void parse_openflow(OpenflowProcess *openflow,
                               const std::string &filename,
                               const Network &network);
};
