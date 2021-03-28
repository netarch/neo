#include "config.hpp"

#include <cassert>
#include <string>
#include <utility>
#include <regex>
#include <typeinfo>
#include <cpptoml.hpp>

#include "interface.hpp"
#include "route.hpp"
#include "node.hpp"
#include "mb-app/mb-app.hpp"
#include "mb-app/netfilter.hpp"
#include "mb-app/ipvs.hpp"
#include "mb-app/squid.hpp"
#include "middlebox.hpp"
#include "link.hpp"
#include "network.hpp"
#include "protocols.hpp"
#include "connspec.hpp"
#include "policy/policy.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "policy/one-request.hpp"
#include "policy/loadbalance.hpp"
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

#define IPV4_PREF_REGEX "\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}/\\d+\\b"
#define IPV4_ADDR_REGEX "\\b(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})(?:[^/]|$)"
#define PORT_REGEX "(?:port\\s+|\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:)(\\d+)\\b"

static const std::regex ip_prefix_regex(IPV4_PREF_REGEX);
static const std::regex ip_addr_regex(IPV4_ADDR_REGEX);
static const std::regex port_regex(PORT_REGEX);

void Config::parse_appliance_config(MB_App *app, const std::string& config)
{
    std::smatch match;
    std::string subject;

    // search for IP prefix patterns
    subject = config;
    while (std::regex_search(subject, match, ip_prefix_regex)) {
        Logger::debug("Found \"" + match.str(0) + "\" when parsing middlebox");
        app->ip_prefixes.emplace(match.str(0));
        subject = match.suffix();
    }

    // search for IP address patterns
    subject = config;
    while (std::regex_search(subject, match, ip_addr_regex)) {
        Logger::debug("Found \"" + match.str(1) + "\" when parsing middlebox");
        app->ip_addrs.emplace(match.str(1));
        subject = match.suffix();
    }

    // search for port patterns
    subject = config;
    while (std::regex_search(subject, match, port_regex)) {
        Logger::debug("Found \"" + match.str(1) + "\" when parsing middlebox");
        app->ports.insert(std::stoul(match.str(1)));
        subject = match.suffix();
    }
}

void Config::parse_netfilter(
    NetFilter *netfilter,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(netfilter != nullptr);

    auto rpf = config->get_as<int>("rp_filter");
    auto rules = config->get_as<std::string>("rules");

    if (!rules) {
        Logger::error("Missing rules");
    }

    if (!rpf) {
        netfilter->rp_filter = 0;
    } else if (*rpf >= 0 && *rpf <= 2) {
        netfilter->rp_filter = *rpf;
    } else {
        Logger::error("Invalid rp_filter value: " + std::to_string(*rpf));
    }
    netfilter->rules = *rules;
    Config::parse_appliance_config(netfilter, netfilter->rules);
}

void Config::parse_ipvs(
    IPVS *ipvs,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(ipvs != nullptr);

    auto ipvs_config = config->get_as<std::string>("config");

    if (!ipvs_config) {
        Logger::error("Missing config");
    }

    ipvs->config = *ipvs_config;
    Config::parse_appliance_config(ipvs, ipvs->config);
}

void Config::parse_squid(
    Squid *squid,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(squid != nullptr);

    auto squid_config = config->get_as<std::string>("config");

    if (!squid_config) {
        Logger::error("Missing config");
    }

    squid->config = *squid_config;
    Config::parse_appliance_config(squid, squid->config);
}

void Config::parse_middlebox(
    Middlebox *middlebox,
    const std::shared_ptr<cpptoml::table>& config)
{
    assert(middlebox != nullptr);

    Config::parse_node(middlebox, config);

    auto environment = config->get_as<std::string>("env");
    auto appliance = config->get_as<std::string>("app");
    auto timeout = config->get_as<long>("timeout");

    if (!environment) {
        Logger::error("Missing environment");
    }
    if (!appliance) {
        Logger::error("Missing appliance");
    }
    if (!timeout) {
        Logger::error("Missing timeout");
    }

    if (*environment == "netns") {
        middlebox->env = *environment;
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

    if (*timeout < 0) {
        Logger::error("Invalid timeout: " + std::to_string(*timeout));
    }
    middlebox->timeout = std::chrono::microseconds(*timeout);
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
                network->add_middlebox(static_cast<Middlebox *>(node));
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

void Config::parse_conn_spec(
    ConnSpec *conn_spec,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(conn_spec != nullptr);

    auto proto_str = config->get_as<std::string>("protocol");
    auto src_node_regex = config->get_as<std::string>("src_node");
    auto dst_ip_str = config->get_as<std::string>("dst_ip");
    auto src_port = config->get_as<int>("src_port");
    auto dst_ports = config->get_array_of<int>("dst_port");
    auto owned_dst_only = config->get_as<bool>("owned_dst_only");

    if (!proto_str) {
        Logger::error("Missing protocol");
    }
    if (!src_node_regex) {
        Logger::error("Missing src_node");
    }
    if (!dst_ip_str) {
        Logger::error("Missing dst_ip");
    }

    std::string proto_s = *proto_str;
    std::transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
    [](unsigned char c) {
        return std::tolower(c);
    });
    if (proto_s == "tcp") {
        conn_spec->protocol = proto::tcp;
    } else if (proto_s == "udp") {
        conn_spec->protocol = proto::udp;
    } else if (proto_s == "icmp-echo") {
        conn_spec->protocol = proto::icmp_echo;
    } else {
        Logger::error("Unknown protocol: " + *proto_str);
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*src_node_regex))) {
            conn_spec->src_nodes.insert(node.second);
        }
    }

    std::string ip_str = *dst_ip_str;
    if (ip_str.find('/') == std::string::npos) {
        ip_str += "/32";
    }
    conn_spec->dst_ip = IPRange<IPv4Address>(ip_str);

    if (conn_spec->protocol == proto::tcp || conn_spec->protocol == proto::udp) {
        conn_spec->src_port = src_port ? *src_port : 49152; // 49152 to 65535
        if (dst_ports) {
            for (const auto& dst_port : *dst_ports) {
                conn_spec->dst_ports.insert(dst_port);
            }
        }
    }

    conn_spec->owned_dst_only = owned_dst_only ? *owned_dst_only : false;
}

void Config::parse_connections(
    Policy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto conns_cfg = config->get_table_array("connections");
    if (!conns_cfg) {
        Logger::error("Missing connections");
    }
    for (const auto& conn_cfg : *conns_cfg) {
        ConnSpec conn;
        Config::parse_conn_spec(&conn, conn_cfg, network);
        policy->conn_specs.push_back(std::move(conn));
    }
}

void Config::parse_reachabilitypolicy(
    ReachabilityPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto target_node_regex = config->get_as<std::string>("target_node");
    auto reachable = config->get_as<bool>("reachable");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!reachable) {
        Logger::error("Missing reachable");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = *reachable;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_replyreachabilitypolicy(
    ReplyReachabilityPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto target_node_regex = config->get_as<std::string>("target_node");
    auto reachable = config->get_as<bool>("reachable");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!reachable) {
        Logger::error("Missing reachable");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = *reachable;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_waypointpolicy(
    WaypointPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto target_node_regex = config->get_as<std::string>("target_node");
    auto pass_through = config->get_as<bool>("pass_through");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!pass_through) {
        Logger::error("Missing pass_through");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->pass_through = *pass_through;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_onerequestpolicy(
    OneRequestPolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto target_node_regex = config->get_as<std::string>("target_node");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
}

void Config::parse_loadbalancepolicy(
    LoadBalancePolicy *policy,
    const std::shared_ptr<cpptoml::table>& config,
    const Network& network)
{
    assert(policy != nullptr);

    auto target_node_regex = config->get_as<std::string>("target_node");
    auto max_vmr = config->get_as<double>("max_dispersion_index");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }

    for (const auto& node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(*target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->max_dispersion_index = max_vmr ? *max_vmr : 1;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
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

    Config::parse_policy_array(policy->correlated_policies, true, policies_config, network);
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

void Config::parse_policy_array(
    std::vector<Policy *>& policies,
    bool correlated,
    const std::shared_ptr<cpptoml::table_array>& policies_config,
    const Network& network)
{
    for (const auto& cfg : *policies_config) {
        Policy *policy = nullptr;

        auto type = cfg->get_as<std::string>("type");
        if (!type) {
            Logger::error("Missing policy type");
        }

        if (*type == "reachability") {
            policy = new ReachabilityPolicy(correlated);
            Config::parse_reachabilitypolicy(
                static_cast<ReachabilityPolicy *>(policy),
                cfg, network);
        } else if (*type == "reply-reachability") {
            policy = new ReplyReachabilityPolicy(correlated);
            Config::parse_replyreachabilitypolicy(
                static_cast<ReplyReachabilityPolicy *>(policy),
                cfg, network);
        } else if (*type == "waypoint") {
            policy = new WaypointPolicy(correlated);
            Config::parse_waypointpolicy(
                static_cast<WaypointPolicy *>(policy),
                cfg, network);
        } else if (!correlated && *type == "one-request") {
            policy = new OneRequestPolicy();
            Config::parse_onerequestpolicy(
                static_cast<OneRequestPolicy *>(policy),
                cfg, network);
        } else if (!correlated && *type == "loadbalance") {
            policy = new LoadBalancePolicy();
            Config::parse_loadbalancepolicy(
                static_cast<LoadBalancePolicy *>(policy),
                cfg, network);
        } else if (!correlated && *type == "conditional") {
            policy = new ConditionalPolicy();
            Config::parse_conditionalpolicy(
                static_cast<ConditionalPolicy *>(policy),
                cfg, network);
        } else if (!correlated && *type == "consistency") {
            policy = new ConsistencyPolicy();
            Config::parse_consistencypolicy(
                static_cast<ConsistencyPolicy *>(policy),
                cfg, network);
        } else {
            Logger::error("Unknown policy type: " + *type);
        }

        assert(policy->conn_specs.size() <= MAX_CONNS);
        policies.push_back(policy);
    }
}

void Config::parse_policies(
    Policies *policies, const std::string& filename, const Network& network)
{
    auto config = configs.at(filename);
    auto policies_config = config->get_table_array("policies");

    if (policies_config) {
        Config::parse_policy_array(policies->policies, false, policies_config, network);
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

    if (openflow->num_nodes() > 0) {
        Logger::info("Loaded " + std::to_string(openflow->num_updates())
                     + " openflow updates for "
                     + std::to_string(openflow->num_nodes()) + " nodes");
    }
}
