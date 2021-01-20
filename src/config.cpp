#include "config.hpp"

#include <cassert>
#include <string>
#include <utility>
#include <regex>
#include <cpptoml.hpp>

#include "interface.hpp"
#include "route.hpp"
#include "node.hpp"
#include "mb-app/mb-app.hpp"
#include "mb-app/netfilter.hpp"
#include "mb-app/ipvs.hpp"
#include "mb-app/squid.hpp"
#include "mb-env/netns.hpp"
#include "middlebox.hpp"
#include "link.hpp"
#include "network.hpp"
#include "comm.hpp"
#include "policy/policy.hpp"
#include "policy/loadbalance.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "policy/one-request.hpp"
#include "policy/conditional.hpp"
#include "policy/consistency.hpp"
#include "process/openflow.hpp"

std::unordered_map<std::string, std::shared_ptr<cpptoml::table>> Config::configs;

void Config::start_parsing(const std::string& filename)
{
    auto res = configs.emplace(filename, cpptoml::parse_file(filename));
    if (!res.second) {
        Logger::error("Duplicate config: " + res.first->first);
    }
    Logger::info("Parsing configuration file " + filename);
}

void Config::finish_parsing(const std::string& filename)
{
    configs.erase(filename);
    Logger::info("Finished parsing");
}

void Config::parse_interface(Interface *interface,
                             const std::shared_ptr<cpptoml::table>& config)
{
    assert(interface != nullptr);

    auto intf_name = config->get_as<std::string>("name");
    auto ipv4_addr = config->get_as<std::string>("ipv4");

    if (!intf_name) {
        Logger::error("Missing interface name");
    }
    interface->name = *intf_name;

    if (ipv4_addr) {
        interface->ipv4 = *ipv4_addr;
        interface->is_switchport = false;
    } else {
        interface->is_switchport = true;
    }
}

void Config::parse_route(Route *route,
                         const std::shared_ptr<cpptoml::table>& config)
{
    assert(route != nullptr);

    auto network = config->get_as<std::string>("network");
    auto next_hop = config->get_as<std::string>("next_hop");
    auto interface = config->get_as<std::string>("interface");
    auto adm_dist = config->get_as<int>("adm_dist");

    if (!network) {
        Logger::error("Missing network");
    }
    if (!next_hop && !interface) {
        Logger::error("Missing next hop IP address and interface");
    }

    route->network = IPNetwork<IPv4Address>(*network);
    if (next_hop) {
        route->next_hop = IPv4Address(*next_hop);
    }
    if (interface) {
        route->egress_intf = std::string(*interface);
    }
    if (adm_dist) {
        if (*adm_dist < 1 || *adm_dist > 254) {
            Logger::error("Invalid administrative distance: " +
                          std::to_string(*adm_dist));
        }
        route->adm_dist = *adm_dist;
    }
}

void Config::parse_node(Node *node,
                        const std::shared_ptr<cpptoml::table>& config)
{
    assert(node != nullptr);

    auto node_name = config->get_as<std::string>("name");
    auto interfaces = config->get_table_array("interfaces");
    auto static_routes = config->get_table_array("static_routes");
    auto installed_routes = config->get_table_array("installed_routes");

    if (!node_name) {
        Logger::error("Missing node name");
    }
    node->name = *node_name;

    if (interfaces) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *interfaces) {
            Interface *interface = new Interface();
            Config::parse_interface(interface, cfg);
            node->add_interface(interface);
        }
    }

    if (static_routes) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *static_routes) {
            Route route;
            Config::parse_route(&route, cfg);
            if (route.get_adm_dist() == 255) { // user did not specify adm dist
                route.adm_dist = 1;
            }
            node->rib.insert(std::move(route));
        }
    }

    if (installed_routes) {
        for (const std::shared_ptr<cpptoml::table>& cfg : *installed_routes) {
            Route route;
            Config::parse_route(&route, cfg);
            node->rib.insert(std::move(route));
        }
    }
}

void Config::parse_mb_app(
    MB_App *app,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(app != nullptr);

    auto timeout = config->get_as<long>("timeout");
    if (!timeout) {
        Logger::error("Missing timeout");
    }

    if (*timeout < 0) {
        Logger::error("Invalid timeout: " + std::to_string(*timeout));
    }
    app->timeout = std::chrono::microseconds(*timeout);
}

void Config::parse_netfilter(
    NetFilter *netfilter,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(netfilter != nullptr);

    Config::parse_mb_app(netfilter, config);

    auto rpf = config->get_as<int>("rp_filter");
    auto rules = config->get_as<std::string>("rules");

    if (!rpf) {
        Logger::error("Missing rp_filter");
    }
    if (!rules) {
        Logger::error("Missing rules");
    }

    if (*rpf < 0 || *rpf > 2) {
        Logger::error("Invalid rp_filter value: " + std::to_string(*rpf));
    }
    netfilter->rp_filter = *rpf;
    netfilter->rules = *rules;
}

void Config::parse_ipvs(
    IPVS *ipvs,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(ipvs != nullptr);

    Config::parse_mb_app(ipvs, config);

    auto ipvs_config = config->get_as<std::string>("config");

    if (!ipvs_config) {
        Logger::error("Missing config");
    }

    ipvs->config = *ipvs_config;
}

void Config::parse_squid(
    Squid *squid,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(squid != nullptr);

    Config::parse_mb_app(squid, config);

    auto squid_config = config->get_as<std::string>("config");

    if (!squid_config) {
        Logger::error("Missing config");
    }

    squid->config = *squid_config;
}

void Config::parse_middlebox(
    Middlebox *middlebox,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(middlebox != nullptr);

    Config::parse_node(middlebox, config);

    auto environment = config->get_as<std::string>("env");
    auto appliance = config->get_as<std::string>("app");

    if (!environment) {
        Logger::error("Missing environment");
    }
    if (!appliance) {
        Logger::error("Missing appliance");
    }

    if (*environment == "netns") {
        middlebox->env = new NetNS();
    } else {
        Logger::error("Unknown environment: " + *environment);
    }

    if (*appliance == "netfilter") {
        middlebox->app = new NetFilter();
        Config::parse_netfilter(static_cast<NetFilter *>(middlebox->app), config);
    } else if (*appliance == "ipvs") {
        middlebox->app = new IPVS();
        Config::parse_ipvs(static_cast<IPVS *>(middlebox->app), config);
    } else if (*appliance == "squid") {
        middlebox->app = new Squid();
        Config::parse_squid(static_cast<Squid *>(middlebox->app), config);
    } else {
        Logger::error("Unknown appliance: " + *appliance);
    }
}

void Config::parse_link(
    Link *link,
    const std::shared_ptr<cpptoml::table>& config,
    const std::map<std::string, Node *>& nodes)
{
    assert(link != nullptr);

    auto node1_name = config->get_as<std::string>("node1");
    auto intf1_name = config->get_as<std::string>("intf1");
    auto node2_name = config->get_as<std::string>("node2");
    auto intf2_name = config->get_as<std::string>("intf2");

    if (!node1_name) {
        Logger::error("Missing node1");
    }
    if (!intf1_name) {
        Logger::error("Missing intf1");
    }
    if (!node2_name) {
        Logger::error("Missing node2");
    }
    if (!intf2_name) {
        Logger::error("Missing intf2");
    }

    auto node1_entry = nodes.find(*node1_name);
    if (node1_entry == nodes.end()) {
        Logger::error("Unknown node: " + *node1_name);
    }
    link->node1 = node1_entry->second;
    auto node2_entry = nodes.find(*node2_name);
    if (node2_entry == nodes.end()) {
        Logger::error("Unknown node: " + *node2_name);
    }
    link->node2 = node2_entry->second;
    link->intf1 = link->node1->get_interface(*intf1_name);
    link->intf2 = link->node2->get_interface(*intf2_name);

    // normalize
    if (link->node1 > link->node2) {
        std::swap(link->node1, link->node2);
        std::swap(link->intf1, link->intf2);
    } else if (link->node1 == link->node2) {
        Logger::error("Invalid link: " + link->to_string());
    }
}

void Config::parse_network(Network *network, const std::string& filename)
{
    assert(network != nullptr);

    auto config = configs.at(filename);
    auto nodes_config = config->get_table_array("nodes");
    auto links_config = config->get_table_array("links");

    if (nodes_config) {
        for (const auto& cfg : *nodes_config) {
            Node *node = nullptr;
            auto type = cfg->get_as<std::string>("type");
            if (!type || *type == "generic") {
                node = new Node();
                Config::parse_node(node, cfg);
            } else if (*type == "middlebox") {
                node = new Middlebox();
                Config::parse_middlebox(static_cast<Middlebox *>(node), cfg);
            } else {
                Logger::error("Unknown node type: " + *type);
            }
            network->add_node(node);
        }
    }
    Logger::info("Loaded " + std::to_string(network->get_nodes().size()) + " nodes");

    if (links_config) {
        for (const auto& cfg : *links_config) {
            Link *link = new Link();
            Config::parse_link(link, cfg, network->get_nodes());
            network->add_link(link);
        }
    }
    Logger::info("Loaded " + std::to_string(network->get_links().size()) + " links");

    // populate L2 LANs (assuming there is no VLAN for now)
    for (const auto& pair : network->get_nodes()) {
        Node *node = pair.second;
        for (Interface *intf : node->get_intfs_l2()) {
            if (!node->mapped_to_l2lan(intf)) {
                network->grow_and_set_l2_lan(node, intf);
            }
        }
    }
}

void Config::parse_communication(
    Communication *comm,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(comm != nullptr);

    auto proto_str = config->get_as<std::string>("protocol");
    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");
    auto owned_only = config->get_as<bool>("owned_dst_only");
    auto start_regex = config->get_as<std::string>("start_node");

    if (!proto_str) {
        Logger::error("Missing protocol");
    }
    if (!pkt_dst_str) {
        Logger::error("Missing packet destination");
    }
    if (!start_regex) {
        Logger::error("Missing start node");
    }

    std::string proto_s = *proto_str;
    std::transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
    [](unsigned char c) {
        return std::tolower(c);
    });
    if (proto_s == "http") {
        comm->protocol = proto::PR_HTTP;
    } else if (proto_s == "icmp-echo") {
        comm->protocol = proto::PR_ICMP_ECHO;
    } else {
        Logger::error("Unknown protocol: " + *proto_str);
    }

    std::string dst_str = *pkt_dst_str;
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    comm->pkt_dst = IPRange<IPv4Address>(dst_str);

    comm->owned_dst_only = owned_only ? *owned_only : false;

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*start_regex))) {
            comm->start_nodes.push_back(node.second);
        }
    }

    // NOTE: fixed port numbers for now
    comm->tx_port = 1234;
    comm->rx_port = 80;
}

void Config::parse_loadbalancepolicy(
    LoadBalancePolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto final_regex = config->get_as<std::string>("final_node");
    auto repeat = config->get_as<int>("repeat");
    auto comm_cfg = config->get_table("communication");

    if (!final_regex) {
        Logger::error("Missing final node");
    }
    if (!comm_cfg) {
        Logger::error("Missing communication");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            policy->final_nodes.insert(node.second);
        }
    }

    if (repeat) {
        policy->repetition = *repeat;
    } else {
        policy->repetition = policy->final_nodes.size();
    }

    Communication comm;
    Config::parse_communication(&comm, comm_cfg, network);
    policy->comms.push_back(std::move(comm));
}

void Config::parse_reachabilitypolicy(
    ReachabilityPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto final_regex = config->get_as<std::string>("final_node");
    auto reachability = config->get_as<bool>("reachable");
    auto comm_cfg = config->get_table("communication");

    if (!final_regex) {
        Logger::error("Missing final node");
    }
    if (!reachability) {
        Logger::error("Missing reachability");
    }
    if (!comm_cfg) {
        Logger::error("Missing communication");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*final_regex))) {
            policy->final_nodes.insert(node.second);
        }
    }

    policy->reachable = *reachability;

    Communication comm;
    Config::parse_communication(&comm, comm_cfg, network);
    policy->comms.push_back(std::move(comm));
}

void Config::parse_replyreachabilitypolicy(
    ReplyReachabilityPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto query_regex = config->get_as<std::string>("query_node");
    auto reachability = config->get_as<bool>("reachable");
    auto comm_cfg = config->get_table("communication");

    if (!query_regex) {
        Logger::error("Missing query node");
    }
    if (!reachability) {
        Logger::error("Missing reachability");
    }
    if (!comm_cfg) {
        Logger::error("Missing communication");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*query_regex))) {
            policy->query_nodes.insert(node.second);
        }
    }

    policy->reachable = *reachability;

    Communication comm;
    Config::parse_communication(&comm, comm_cfg, network);
    policy->comms.push_back(std::move(comm));
}

void Config::parse_waypointpolicy(
    WaypointPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto wp_regex = config->get_as<std::string>("waypoint");
    auto through = config->get_as<bool>("pass_through");
    auto comm_cfg = config->get_table("communication");

    if (!wp_regex) {
        Logger::error("Missing waypoint");
    }
    if (!through) {
        Logger::error("Missing pass_through");
    }
    if (!comm_cfg) {
        Logger::error("Missing communication");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*wp_regex))) {
            policy->waypoints.insert(node.second);
        }
    }

    policy->pass_through = *through;

    Communication comm;
    Config::parse_communication(&comm, comm_cfg, network);
    policy->comms.push_back(std::move(comm));
}

void Config::parse_onerequestpolicy(
    OneRequestPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto server_regex = config->get_as<std::string>("server_node");
    auto comms_cfg = config->get_table_array("communications");

    if (!server_regex) {
        Logger::error("Missing server node");
    }
    if (!comms_cfg) {
        Logger::error("Missing communications");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*server_regex))) {
            policy->server_nodes.insert(node.second);
        }
    }

    for (const auto& comm_cfg : *comms_cfg) {
        Communication comm;
        Config::parse_communication(&comm, comm_cfg, network);
        policy->comms.push_back(std::move(comm));
    }
}

void Config::parse_correlated_policies(
    Policy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto policies_config = config->get_table_array("correlated_policies");

    if (!policies_config) {
        Logger::error("Missing correlated policies");
    }

    for (const auto& cfg : *policies_config) {
        Policy *correlated_policy = nullptr;

        auto type = config->get_as<std::string>("type");
        if (!type) {
            Logger::error("Missing policy type");
        }

        if (*type == "loadbalance") {
            correlated_policy = new LoadBalancePolicy(true);
            Config::parse_loadbalancepolicy(
                static_cast<LoadBalancePolicy *>(correlated_policy),
                cfg, network);
        } else if (*type == "reachability") {
            correlated_policy = new ReachabilityPolicy(true);
            Config::parse_reachabilitypolicy(
                static_cast<ReachabilityPolicy *>(correlated_policy),
                cfg, network);
        } else if (*type == "reply-reachability") {
            correlated_policy = new ReplyReachabilityPolicy(true);
            Config::parse_replyreachabilitypolicy(
                static_cast<ReplyReachabilityPolicy *>(correlated_policy),
                cfg, network);
        } else if (*type == "waypoint") {
            correlated_policy = new WaypointPolicy(true);
            Config::parse_waypointpolicy(
                static_cast<WaypointPolicy *>(correlated_policy),
                cfg, network);
        } else {
            Logger::error("Unsupported policy type: " + *type);
        }

        policy->correlated_policies.push_back(correlated_policy);
    }
}

void Config::parse_conditionalpolicy(
    ConditionalPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    parse_correlated_policies(policy, config, network);
}

void Config::parse_consistencypolicy(
    ConsistencyPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    parse_correlated_policies(policy, config, network);
}

void Config::parse_policies(
    Policies *policies, const std::string& filename, const Network& network)
{
    auto config = configs.at(filename);
    auto policies_config = config->get_table_array("policies");

    if (policies_config) {
        for (const auto& cfg : *policies_config) {
            Policy *policy = nullptr;

            auto type = cfg->get_as<std::string>("type");
            if (!type) {
                Logger::error("Missing policy type");
            }

            if (*type == "loadbalance") {
                policy = new LoadBalancePolicy();
                Config::parse_loadbalancepolicy(
                    static_cast<LoadBalancePolicy *>(policy),
                    cfg, network);
            } else if (*type == "reachability") {
                policy = new ReachabilityPolicy();
                Config::parse_reachabilitypolicy(
                    static_cast<ReachabilityPolicy *>(policy),
                    cfg, network);
            } else if (*type == "reply-reachability") {
                policy = new ReplyReachabilityPolicy();
                Config::parse_replyreachabilitypolicy(
                    static_cast<ReplyReachabilityPolicy *>(policy),
                    cfg, network);
            } else if (*type == "waypoint") {
                policy = new WaypointPolicy();
                Config::parse_waypointpolicy(
                    static_cast<WaypointPolicy *>(policy),
                    cfg, network);
            } else if (*type == "one-request") {
                policy = new OneRequestPolicy();
                Config::parse_onerequestpolicy(
                    static_cast<OneRequestPolicy *>(policy),
                    cfg, network);
            } else if (*type == "conditional") {
                policy = new ConditionalPolicy();
                Config::parse_conditionalpolicy(
                    static_cast<ConditionalPolicy *>(policy),
                    cfg, network);
            } else if (*type == "consistency") {
                policy = new ConsistencyPolicy();
                Config::parse_consistencypolicy(
                    static_cast<ConsistencyPolicy *>(policy),
                    cfg, network);
            } else {
                Logger::error("Unknown policy type: " + *type);
            }

            policies->policies.push_back(policy);
        }
    }

    if (policies->policies.size() == 1) {
        Logger::info("Loaded 1 policy");
    } else {
        Logger::info("Loaded " + std::to_string(policies->policies.size()) + " policies");
    }
}

void Config::parse_openflow_update(
    Node **node,
    Route *route,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(node != nullptr);
    assert(route != nullptr);

    auto node_name = config->get_as<std::string>("node");
    auto network_cidr = config->get_as<std::string>("network");
    auto outport = config->get_as<std::string>("outport");

    if (!node_name) {
        Logger::error("Missing node name");
    }
    if (!network_cidr) {
        Logger::error("Missing network");
    }
    if (!outport) {
        Logger::error("Missing outport");
    }

    auto node_itr = network.get_nodes().find(*node_name);
    if (node_itr == network.get_nodes().end()) {
        Logger::error("Unknown node: " + *node_name);
    }

    *node = node_itr->second;
    if (typeid(**node) != typeid(Node)) {
        Logger::error("Unsupported node type");
    }

    route->network = IPNetwork<IPv4Address>(*network_cidr);
    route->egress_intf = std::string(*outport);
}

void Config::parse_openflow(OpenflowProcess *openflow,
                            const std::string& filename,
                            const Network& network)
{
    auto config = configs.at(filename);
    auto openflow_cfg = config->get_table("openflow");

    if (!openflow_cfg) {
        return;
    }

    auto updates_cfg = openflow_cfg->get_table_array("updates");

    if (!updates_cfg) {
        return;
    }

    for (const auto& update_cfg : *updates_cfg) {
        Node *node;
        Route route;
        Config::parse_openflow_update(&node, &route, update_cfg, network);
        openflow->add_update(node, std::move(route));
    }
}
