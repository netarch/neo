#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include <toml++/toml.h>

class Conditional;
class ConnSpec;
class Consistency;
class DockerNode;
class Interface;
class Link;
class LoadBalance;
class Middlebox;
class Network;
class Node;
class OneRequest;
class OpenflowProcess;
class Plankton;
class Invariant;
class Reachability;
class ReplyReachability;
class Route;
class Waypoint;

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

    // Invariants
    void parse_invariants(std::vector<std::shared_ptr<Invariant>> &invariants,
                          const Network &network);
    void parse_inv_array(std::vector<std::shared_ptr<Invariant>> &invariants,
                         bool correlated,
                         const toml::array &inv_arr,
                         const Network &network);
    // Single-connection invariants
    void parse_reachability(std::shared_ptr<Reachability> &,
                            const toml::table &,
                            const Network &);
    void parse_replyreachability(std::shared_ptr<ReplyReachability> &,
                                 const toml::table &,
                                 const Network &);
    void parse_waypoint(std::shared_ptr<Waypoint> &,
                        const toml::table &,
                        const Network &);
    // Multi-connection invariants
    void parse_onerequest(std::shared_ptr<OneRequest> &,
                          const toml::table &,
                          const Network &);
    void parse_loadbalance(std::shared_ptr<LoadBalance> &,
                           const toml::table &,
                           const Network &);
    // Invariants with multiple correlated sub-invariants
    void parse_conditional(std::shared_ptr<Conditional> &,
                           const toml::table &,
                           const Network &);
    void parse_consistency(std::shared_ptr<Consistency> &,
                           const toml::table &,
                           const Network &);
    void parse_correlated_invs(std::shared_ptr<Invariant>,
                               const toml::table &,
                               const Network &);
    // Connections
    void parse_connections(std::shared_ptr<Invariant>,
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
