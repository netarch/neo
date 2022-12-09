#include "config.hpp"

#include <cassert>
#include <csignal>
#include <memory>
#include <regex>
#include <string>
#include <toml++/toml.h>
#include <typeinfo>
#include <utility>

#include "connspec.hpp"
#include "interface.hpp"
#include "link.hpp"
#include "mb-app/ipvs.hpp"
#include "mb-app/mb-app.hpp"
#include "mb-app/netfilter.hpp"
#include "mb-app/squid.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "node.hpp"
#include "policy/conditional.hpp"
#include "policy/consistency.hpp"
#include "policy/loadbalance.hpp"
#include "policy/one-request.hpp"
#include "policy/policy.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/waypoint.hpp"
#include "process/openflow.hpp"
#include "protocols.hpp"
#include "route.hpp"
#include "stats.hpp"

std::unordered_map<std::string, std::shared_ptr<toml::table>> Config::configs;
std::chrono::microseconds Config::latency_avg;
std::chrono::microseconds Config::latency_mdev;
bool Config::got_latency_estimate = false;

void Config::start_parsing(const std::string &filename) {
    auto config = std::make_shared<toml::table>(toml::parse_file(filename));
    auto res = configs.emplace(filename, std::move(config));
    if (!res.second) {
        Logger::error("Duplicate config: " + res.first->first);
    }
    Logger::info("Parsing configuration file " + filename);
}

void Config::finish_parsing(const std::string &filename) {
    configs.erase(filename);
    Logger::info("Finished parsing");
}

void Config::parse_interface(Interface *interface, const toml::table &config) {
    assert(interface != nullptr);

    auto intf_name = config.get_as<std::string>("name");
    auto ipv4_addr = config.get_as<std::string>("ipv4");

    if (!intf_name) {
        Logger::error("Missing interface name");
    }

    interface->name = **intf_name;

    if (ipv4_addr) {
        interface->ipv4 = **ipv4_addr;
        interface->is_switchport = false;
    } else {
        interface->is_switchport = true;
    }
}

void Config::parse_route(Route *route, const toml::table &config) {
    assert(route != nullptr);

    auto network = config.get_as<std::string>("network");
    auto next_hop = config.get_as<std::string>("next_hop");
    auto interface = config.get_as<std::string>("interface");
    auto adm_dist = config.get_as<int64_t>("adm_dist");

    if (!network) {
        Logger::error("Missing network");
    }
    if (!next_hop && !interface) {
        Logger::error("Missing next hop IP address and interface");
    }

    route->network = IPNetwork<IPv4Address>(**network);
    if (next_hop) {
        route->next_hop = IPv4Address(**next_hop);
    }
    if (interface) {
        route->egress_intf = std::string(**interface);
    }
    if (adm_dist) {
        if (**adm_dist < 1 || **adm_dist > 254) {
            Logger::error("Invalid administrative distance: " +
                          std::to_string(**adm_dist));
        }
        route->adm_dist = **adm_dist;
    }
}

void Config::parse_node(Node *node, const toml::table &config) {
    assert(node != nullptr);

    auto node_name = config.get_as<std::string>("name");
    auto interfaces = config.get_as<toml::array>("interfaces");
    auto static_routes = config.get_as<toml::array>("static_routes");
    auto installed_routes = config.get_as<toml::array>("installed_routes");

    if (!node_name) {
        Logger::error("Missing node name");
    }
    node->name = **node_name;

    if (interfaces) {
        for (const auto &cfg : *interfaces) {
            Interface *interface = new Interface();
            Config::parse_interface(interface, *cfg.as_table());
            node->add_interface(interface);
        }
    }

    if (static_routes) {
        for (const auto &cfg : *static_routes) {
            Route route;
            Config::parse_route(&route, *cfg.as_table());
            if (route.get_adm_dist() == 255) { // user did not specify adm dist
                route.adm_dist = 1;
            }
            node->rib.insert(std::move(route));
        }
    }

    if (installed_routes) {
        for (const auto &cfg : *installed_routes) {
            Route route;
            Config::parse_route(&route, *cfg.as_table());
            node->rib.insert(std::move(route));
        }
    }
}

#define IPV4_PREF_REGEX "\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}/\\d+\\b"
#define IPV4_ADDR_REGEX                                                        \
    "\\b(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})(?:[^/]|$)"
#define PORT_REGEX                                                             \
    "(?:port\\s+|\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:)(\\d+)\\b"

static const std::regex ip_prefix_regex(IPV4_PREF_REGEX);
static const std::regex ip_addr_regex(IPV4_ADDR_REGEX);
static const std::regex port_regex(PORT_REGEX);

void Config::parse_appliance_config(MB_App *app, const std::string &config) {
    std::smatch match;
    std::string subject;

    // search for IP prefix patterns
    subject = config;
    while (std::regex_search(subject, match, ip_prefix_regex)) {
        app->ip_prefixes.emplace(match.str(0));
        subject = match.suffix();
    }

    // search for IP address patterns
    subject = config;
    while (std::regex_search(subject, match, ip_addr_regex)) {
        app->ip_addrs.emplace(match.str(1));
        subject = match.suffix();
    }

    // search for port patterns
    subject = config;
    while (std::regex_search(subject, match, port_regex)) {
        app->ports.insert(std::stoul(match.str(1)));
        subject = match.suffix();
    }
}

void Config::parse_netfilter(NetFilter *netfilter, const toml::table &config) {
    assert(netfilter != nullptr);

    auto rpf = config.get_as<int64_t>("rp_filter");
    auto rules = config.get_as<std::string>("rules");

    if (!rules) {
        Logger::error("Missing rules");
    }

    if (!rpf) {
        netfilter->rp_filter = 0;
    } else if (**rpf >= 0 && **rpf <= 2) {
        netfilter->rp_filter = **rpf;
    } else {
        Logger::error("Invalid rp_filter value: " + std::to_string(**rpf));
    }

    netfilter->rules = **rules;
    Config::parse_appliance_config(netfilter, netfilter->rules);
}

void Config::parse_ipvs(IPVS *ipvs, const toml::table &config) {
    assert(ipvs != nullptr);

    auto ipvs_config = config.get_as<std::string>("config");

    if (!ipvs_config) {
        Logger::error("Missing config");
    }

    ipvs->config = **ipvs_config;
    Config::parse_appliance_config(ipvs, ipvs->config);
}

void Config::parse_squid(Squid *squid, const toml::table &config) {
    assert(squid != nullptr);

    auto squid_config = config.get_as<std::string>("config");

    if (!squid_config) {
        Logger::error("Missing config");
    }

    squid->config = **squid_config;
    Config::parse_appliance_config(squid, squid->config);
}

void Config::parse_middlebox(Middlebox *middlebox,
                             const toml::table &config,
                             bool dropmon) {
    assert(middlebox != nullptr);

    if (!Config::got_latency_estimate && !dropmon) {
        Config::estimate_latency();
    }

    Config::parse_node(middlebox, config);

    auto environment = config.get_as<std::string>("env");
    auto appliance = config.get_as<std::string>("app");

    if (!environment) {
        Logger::error("Missing environment");
    }
    if (!appliance) {
        Logger::error("Missing appliance");
    }

    if (**environment == "netns") {
        middlebox->env = **environment;
    } else {
        Logger::error("Unknown environment: " + **environment);
    }

    if (**appliance == "netfilter") {
        middlebox->app = new NetFilter();
        Config::parse_netfilter(static_cast<NetFilter *>(middlebox->app),
                                config);
    } else if (**appliance == "ipvs") {
        middlebox->app = new IPVS();
        Config::parse_ipvs(static_cast<IPVS *>(middlebox->app), config);
    } else if (**appliance == "squid") {
        middlebox->app = new Squid();
        Config::parse_squid(static_cast<Squid *>(middlebox->app), config);
    } else {
        Logger::error("Unknown appliance: " + **appliance);
    }

    middlebox->latency_avg = Config::latency_avg;
    middlebox->latency_mdev = Config::latency_mdev;
    middlebox->dropmon = dropmon;
}

void Config::estimate_latency() {
    /*
     * [192.168.1.2/24]                        [192.168.2.2/24]
     * (node1)-------------------(mb)-------------------(node2)
     *            [192.168.1.1/24]  [192.168.2.1/24]
     */
    Network network;

    Middlebox *mb = new Middlebox(false);
    mb->name = "mb";
    Interface *mb_eth0 = new Interface();
    mb_eth0->name = "eth0";
    mb_eth0->ipv4 = "192.168.1.1/24";
    mb_eth0->is_switchport = false;
    Interface *mb_eth1 = new Interface();
    mb_eth1->name = "eth1";
    mb_eth1->ipv4 = "192.168.2.1/24";
    mb_eth1->is_switchport = false;
    mb->add_interface(mb_eth0);
    mb->add_interface(mb_eth1);
    mb->env = "netns";
    NetFilter *nf = new NetFilter();
    mb->app = nf;
    nf->rp_filter = 0;
    nf->rules = "*filter\n"
                ":INPUT ACCEPT [0:0]\n"
                ":FORWARD ACCEPT [0:0]\n"
                ":OUTPUT ACCEPT [0:0]\n"
                "COMMIT\n";
    Config::parse_appliance_config(nf, nf->rules);
    network.add_middlebox(mb);
    network.add_node(mb);

    Node *node1 = new Node(), *node2 = new Node();
    node1->name = "node1";
    node2->name = "node2";
    Interface *node1_eth0 = new Interface();
    node1_eth0->name = "eth0";
    node1_eth0->ipv4 = "192.168.1.2/24";
    node1_eth0->is_switchport = false;
    Interface *node2_eth0 = new Interface();
    node2_eth0->name = "eth0";
    node2_eth0->ipv4 = "192.168.2.2/24";
    node2_eth0->is_switchport = false;
    node1->add_interface(node1_eth0);
    node2->add_interface(node2_eth0);
    network.add_node(node1);
    network.add_node(node2);

    Link *link = new Link();
    link->node1 = mb;
    link->node2 = node1;
    link->intf1 = mb_eth0;
    link->intf2 = node1_eth0;
    if (link->node1 > link->node2) {
        std::swap(link->node1, link->node2);
        std::swap(link->intf1, link->intf2);
    }
    network.add_link(link);
    link = new Link();
    link->node1 = mb;
    link->node2 = node2;
    link->intf1 = mb_eth1;
    link->intf2 = node2_eth0;
    if (link->node1 > link->node2) {
        std::swap(link->node1, link->node2);
        std::swap(link->intf1, link->intf2);
    }
    network.add_link(link);

    // register signal handler
    struct sigaction action, *oldaction = nullptr;
    action.sa_handler = [](int) {};
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGUSR1);
    action.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGUSR1, &action, oldaction);

    Emulation emulation;
    emulation.init(mb);
    mb->emulation = &emulation;

    // inject packets
    Packet packet(mb_eth0, "192.168.1.2", "192.168.2.2", 49152, 80, 0, 0,
                  PS_TCP_INIT_1);
    assert(emulation.dropmon == false);
    emulation.dropmon = true; // temporarily disable timeout
    for (int i = 0; i < 10; ++i) {
        emulation.send_pkt(packet);
        packet.set_src_port(packet.get_src_port() + 1);
    }
    emulation.dropmon = false;

    // calculate latency average and mean deviation
    const std::vector<std::pair<uint64_t, uint64_t>> &pkt_latencies =
        Stats::get_pkt_latencies();
    long long avg = 0;
    for (const auto &lat : pkt_latencies) {
        avg += lat.second / 1000 + 1;
    }
    avg /= pkt_latencies.size();
    Config::latency_avg = std::chrono::microseconds(avg);
    long long mdev = 0;
    for (const auto &lat : pkt_latencies) {
        mdev += std::abs((long long)(lat.second / 1000 + 1) - avg);
    }
    mdev /= pkt_latencies.size();
    Config::latency_mdev = std::chrono::microseconds(mdev);

    // reset signal handler
    sigaction(SIGUSR1, oldaction, nullptr);

    emulation.teardown();
    Stats::clear_latencies();
    Config::got_latency_estimate = true;
}

void Config::parse_link(Link *link,
                        const toml::table &config,
                        const std::map<std::string, Node *> &nodes) {
    assert(link != nullptr);

    auto node1_name = config.get_as<std::string>("node1");
    auto intf1_name = config.get_as<std::string>("intf1");
    auto node2_name = config.get_as<std::string>("node2");
    auto intf2_name = config.get_as<std::string>("intf2");

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

    auto node1_entry = nodes.find(**node1_name);
    if (node1_entry == nodes.end()) {
        Logger::error("Unknown node: " + **node1_name);
    }
    link->node1 = node1_entry->second;
    auto node2_entry = nodes.find(**node2_name);
    if (node2_entry == nodes.end()) {
        Logger::error("Unknown node: " + **node2_name);
    }
    link->node2 = node2_entry->second;
    link->intf1 = link->node1->get_interface(**intf1_name);
    link->intf2 = link->node2->get_interface(**intf2_name);

    // normalize
    if (link->node1 > link->node2) {
        std::swap(link->node1, link->node2);
        std::swap(link->intf1, link->intf2);
    } else if (link->node1 == link->node2) {
        Logger::error("Invalid link: " + link->to_string());
    }
}

void Config::parse_network(Network *network,
                           const std::string &filename,
                           bool dropmon) {
    assert(network != nullptr);

    auto config = configs.at(filename);
    auto nodes_config = config->get_as<toml::array>("nodes");
    auto links_config = config->get_as<toml::array>("links");

    if (nodes_config) {
        for (const auto &cfg : *nodes_config) {
            const toml::table &tbl = *cfg.as_table();
            Node *node = nullptr;
            auto type = tbl.get_as<std::string>("type");
            if (!type || **type == "generic") {
                node = new Node();
                Config::parse_node(node, tbl);
            } else if (**type == "middlebox") {
                node = new Middlebox(true);
                Config::parse_middlebox(static_cast<Middlebox *>(node), tbl,
                                        dropmon);
                network->add_middlebox(static_cast<Middlebox *>(node));
            } else {
                Logger::error("Unknown node type: " + **type);
            }
            network->add_node(node);
        }
    }
    Logger::info("Loaded " + std::to_string(network->get_nodes().size()) +
                 " nodes");

    if (links_config) {
        for (const auto &cfg : *links_config) {
            const toml::table &tbl = *cfg.as_table();
            Link *link = new Link();
            Config::parse_link(link, tbl, network->get_nodes());
            network->add_link(link);
        }
    }
    Logger::info("Loaded " + std::to_string(network->get_links().size()) +
                 " links");

    // populate L2 LANs (assuming there is no VLAN for now)
    for (const auto &pair : network->get_nodes()) {
        Node *node = pair.second;
        for (Interface *intf : node->get_intfs_l2()) {
            if (!node->mapped_to_l2lan(intf)) {
                network->grow_and_set_l2_lan(node, intf);
            }
        }
    }
}

void Config::parse_conn_spec(ConnSpec *conn_spec,
                             const toml::table &config,
                             const Network &network) {
    assert(conn_spec != nullptr);

    auto proto_str = config.get_as<std::string>("protocol");
    auto src_node_regex = config.get_as<std::string>("src_node");
    auto dst_ip_str = config.get_as<std::string>("dst_ip");
    auto src_port = config.get_as<int64_t>("src_port");
    auto dst_ports = config.get_as<toml::array>("dst_port");
    auto owned_dst_only = config.get_as<bool>("owned_dst_only");

    if (!proto_str) {
        Logger::error("Missing protocol");
    }
    if (!src_node_regex) {
        Logger::error("Missing src_node");
    }
    if (!dst_ip_str) {
        Logger::error("Missing dst_ip");
    }

    std::string proto_s = **proto_str;
    std::transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (proto_s == "tcp") {
        conn_spec->protocol = proto::tcp;
    } else if (proto_s == "udp") {
        conn_spec->protocol = proto::udp;
    } else if (proto_s == "icmp-echo") {
        conn_spec->protocol = proto::icmp_echo;
    } else {
        Logger::error("Unknown protocol: " + **proto_str);
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**src_node_regex))) {
            conn_spec->src_nodes.insert(node.second);
        }
    }

    std::string ip_str = **dst_ip_str;
    if (ip_str.find('/') == std::string::npos) {
        ip_str += "/32";
    }
    conn_spec->dst_ip = IPRange<IPv4Address>(ip_str);

    if (conn_spec->protocol == proto::tcp ||
        conn_spec->protocol == proto::udp) {
        conn_spec->src_port = src_port ? **src_port : DYNAMIC_PORT;
        if (dst_ports) {
            for (const auto &dst_port : *dst_ports) {
                conn_spec->dst_ports.insert(**dst_port.as_integer());
            }
        }
    }

    conn_spec->owned_dst_only = owned_dst_only ? **owned_dst_only : false;
}

void Config::parse_connections(Policy *policy,
                               const toml::table &config,
                               const Network &network) {
    assert(policy != nullptr);

    auto conns_cfg = config.get_as<toml::array>("connections");
    if (!conns_cfg) {
        Logger::error("Missing connections");
    }
    for (const auto &conn_cfg : *conns_cfg) {
        ConnSpec conn;
        Config::parse_conn_spec(&conn, *conn_cfg.as_table(), network);
        policy->conn_specs.push_back(std::move(conn));
    }
}

void Config::parse_reachabilitypolicy(ReachabilityPolicy *policy,
                                      const toml::table &config,
                                      const Network &network) {
    assert(policy != nullptr);

    auto target_node_regex = config.get_as<std::string>("target_node");
    auto reachable = config.get_as<bool>("reachable");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!reachable) {
        Logger::error("Missing reachable");
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = **reachable;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_replyreachabilitypolicy(ReplyReachabilityPolicy *policy,
                                           const toml::table &config,
                                           const Network &network) {
    assert(policy != nullptr);

    auto target_node_regex = config.get_as<std::string>("target_node");
    auto reachable = config.get_as<bool>("reachable");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!reachable) {
        Logger::error("Missing reachable");
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = **reachable;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_waypointpolicy(WaypointPolicy *policy,
                                  const toml::table &config,
                                  const Network &network) {
    assert(policy != nullptr);

    auto target_node_regex = config.get_as<std::string>("target_node");
    auto pass_through = config.get_as<bool>("pass_through");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }
    if (!pass_through) {
        Logger::error("Missing pass_through");
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->pass_through = **pass_through;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void Config::parse_onerequestpolicy(OneRequestPolicy *policy,
                                    const toml::table &config,
                                    const Network &network) {
    assert(policy != nullptr);

    auto target_node_regex = config.get_as<std::string>("target_node");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
}

void Config::parse_loadbalancepolicy(LoadBalancePolicy *policy,
                                     const toml::table &config,
                                     const Network &network) {
    assert(policy != nullptr);

    auto target_node_regex = config.get_as<std::string>("target_node");
    auto max_vmr = config.get_as<double>("max_dispersion_index");

    if (!target_node_regex) {
        Logger::error("Missing target_node");
    }

    for (const auto &node : network.get_nodes()) {
        if (std::regex_match(node.first, std::regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->max_dispersion_index = max_vmr ? **max_vmr : 1;
    Config::parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
}

void Config::parse_correlated_policies(Policy *policy,
                                       const toml::table &config,
                                       const Network &network) {
    assert(policy != nullptr);

    auto policies_config = config.get_as<toml::array>("correlated_policies");

    if (!policies_config) {
        Logger::error("Missing correlated policies");
    }

    Config::parse_policy_array(policy->correlated_policies, true,
                               *policies_config, network);
}

void Config::parse_conditionalpolicy(ConditionalPolicy *policy,
                                     const toml::table &config,
                                     const Network &network) {
    assert(policy != nullptr);

    parse_correlated_policies(policy, config, network);
}

void Config::parse_consistencypolicy(ConsistencyPolicy *policy,
                                     const toml::table &config,
                                     const Network &network) {
    assert(policy != nullptr);

    parse_correlated_policies(policy, config, network);
}

void Config::parse_policy_array(std::vector<Policy *> &policies,
                                bool correlated,
                                const toml::array &policies_config,
                                const Network &network) {
    for (const auto &cfg : policies_config) {
        const toml::table &tbl = *cfg.as_table();
        Policy *policy = nullptr;

        auto type = tbl.get_as<std::string>("type");
        if (!type) {
            Logger::error("Missing policy type");
        }

        if (**type == "reachability") {
            policy = new ReachabilityPolicy(correlated);
            Config::parse_reachabilitypolicy(
                static_cast<ReachabilityPolicy *>(policy), tbl, network);
        } else if (*type == "reply-reachability") {
            policy = new ReplyReachabilityPolicy(correlated);
            Config::parse_replyreachabilitypolicy(
                static_cast<ReplyReachabilityPolicy *>(policy), tbl, network);
        } else if (*type == "waypoint") {
            policy = new WaypointPolicy(correlated);
            Config::parse_waypointpolicy(static_cast<WaypointPolicy *>(policy),
                                         tbl, network);
        } else if (!correlated && *type == "one-request") {
            policy = new OneRequestPolicy();
            Config::parse_onerequestpolicy(
                static_cast<OneRequestPolicy *>(policy), tbl, network);
        } else if (!correlated && *type == "loadbalance") {
            policy = new LoadBalancePolicy();
            Config::parse_loadbalancepolicy(
                static_cast<LoadBalancePolicy *>(policy), tbl, network);
        } else if (!correlated && *type == "conditional") {
            policy = new ConditionalPolicy();
            Config::parse_conditionalpolicy(
                static_cast<ConditionalPolicy *>(policy), tbl, network);
        } else if (!correlated && *type == "consistency") {
            policy = new ConsistencyPolicy();
            Config::parse_consistencypolicy(
                static_cast<ConsistencyPolicy *>(policy), tbl, network);
        } else {
            Logger::error("Unknown policy type: " + **type);
        }

        assert(policy->conn_specs.size() <= MAX_CONNS);
        policies.push_back(policy);
    }
}

void Config::parse_policies(Policies *policies,
                            const std::string &filename,
                            const Network &network) {
    auto config = configs.at(filename);
    auto policies_config = config->get_as<toml::array>("policies");

    if (policies_config) {
        Config::parse_policy_array(policies->policies, false, *policies_config,
                                   network);
    }

    if (policies->policies.size() == 1) {
        Logger::info("Loaded 1 policy");
    } else {
        Logger::info("Loaded " + std::to_string(policies->policies.size()) +
                     " policies");
    }
}

void Config::parse_openflow_update(Node **node,
                                   Route *route,
                                   const toml::table &config,
                                   const Network &network) {
    assert(node != nullptr);
    assert(route != nullptr);

    auto node_name = config.get_as<std::string>("node");
    auto network_cidr = config.get_as<std::string>("network");
    auto outport = config.get_as<std::string>("outport");

    if (!node_name) {
        Logger::error("Missing node name");
    }
    if (!network_cidr) {
        Logger::error("Missing network");
    }
    if (!outport) {
        Logger::error("Missing outport");
    }

    auto node_itr = network.get_nodes().find(**node_name);
    if (node_itr == network.get_nodes().end()) {
        Logger::error("Unknown node: " + **node_name);
    }

    *node = node_itr->second;
    if (typeid(**node) != typeid(Node)) {
        Logger::error("Unsupported node type");
    }

    route->network = IPNetwork<IPv4Address>(**network_cidr);
    route->egress_intf = std::string(**outport);
}

void Config::parse_openflow(OpenflowProcess *openflow,
                            const std::string &filename,
                            const Network &network) {
    auto config = configs.at(filename);
    auto openflow_cfg = config->get_as<toml::table>("openflow");

    if (!openflow_cfg) {
        return;
    }

    auto updates_cfg = openflow_cfg->get_as<toml::array>("updates");

    if (!updates_cfg) {
        return;
    }

    for (const auto &update_cfg : *updates_cfg) {
        Node *node;
        Route route;
        Config::parse_openflow_update(&node, &route, *update_cfg.as_table(),
                                      network);
        openflow->add_update(node, std::move(route));
    }

    if (openflow->num_nodes() > 0) {
        Logger::info("Loaded " + std::to_string(openflow->num_updates()) +
                     " openflow updates for " +
                     std::to_string(openflow->num_nodes()) + " nodes");
    }
}
