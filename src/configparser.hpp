#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <toml++/toml.h>

class ConditionalPolicy;
class ConnSpec;
class ConsistencyPolicy;
class DockerNode;
class Interface;
class Link;
class LoadBalancePolicy;
class Middlebox;
class Network;
class Node;
class OneRequestPolicy;
class OpenflowProcess;
class Plankton;
class Policy;
class ReachabilityPolicy;
class ReplyReachabilityPolicy;
class Route;
class WaypointPolicy;

class ConfigParser {
private:
    // Config file table
    std::unique_ptr<toml::table> _config;

    // Network model
    void parse_network(Network &);
    void parse_node(Node &, const toml::table &);
    void parse_interface(Interface &, const toml::table &);
    void parse_route(Route &, const toml::table &);

    // Network middleboxes
    void parse_dockernode(DockerNode &, const toml::table &);
    void parse_middlebox(Middlebox &, const toml::table &);

    // Initial estimate of the packet injection latency
    void estimate_pkt_lat();

    // Parse middlebox configs for calculating PECs
    void parse_config_string(Middlebox &, const std::string &);

    // Network link
    void parse_link(Link &,
                    const toml::table &,
                    const std::unordered_map<std::string, Node *> &);

    // Openflow
    void parse_openflow(OpenflowProcess &, const Network &);
    void parse_openflow_update(Node *&,
                               Route &,
                               const toml::table &,
                               const Network &);

    // Policies
    void parse_policies(std::vector<std::shared_ptr<Policy>> &policies,
                        const Network &network);
    void parse_policy_array(std::vector<std::shared_ptr<Policy>> &policies,
                            bool correlated,
                            const toml::array &pol_arr,
                            const Network &network);
    // Single-connection policies
    void parse_reachability(std::shared_ptr<ReachabilityPolicy> &,
                            const toml::table &,
                            const Network &);
    void parse_replyreachability(std::shared_ptr<ReplyReachabilityPolicy> &,
                                 const toml::table &,
                                 const Network &);
    void parse_waypoint(std::shared_ptr<WaypointPolicy> &,
                        const toml::table &,
                        const Network &);
    // Multi-connection policies
    void parse_onerequest(std::shared_ptr<OneRequestPolicy> &,
                          const toml::table &,
                          const Network &);
    void parse_loadbalance(std::shared_ptr<LoadBalancePolicy> &,
                           const toml::table &,
                           const Network &);
    // Policies with multiple correlated sub-policies
    void parse_conditional(std::shared_ptr<ConditionalPolicy> &,
                           const toml::table &,
                           const Network &);
    void parse_consistency(std::shared_ptr<ConsistencyPolicy> &,
                           const toml::table &,
                           const Network &);
    void parse_correlated_policies(std::shared_ptr<Policy>,
                                   const toml::table &,
                                   const Network &);
    // Connections
    void parse_connections(std::shared_ptr<Policy>,
                           const toml::table &,
                           const Network &);
    void parse_conn_spec(ConnSpec &, const toml::table &, const Network &);

public:
    ConfigParser() = default;
    ConfigParser(const ConfigParser &) = delete;
    ConfigParser(ConfigParser &&) = delete;
    ConfigParser &operator=(const ConfigParser &) = delete;
    ConfigParser &operator=(ConfigParser &&) = delete;

    void parse(const std::string &filename, Plankton &plankton);
};
