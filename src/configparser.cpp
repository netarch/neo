#include "configparser.hpp"

#include <fstream>
#include <regex>
#include <sstream>

#include <boost/filesystem.hpp>
#include <toml++/toml.h>

#include "connspec.hpp"
#include "dockerapi.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "interface.hpp"
#include "link.hpp"
#include "logger.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "node.hpp"
#include "plankton.hpp"
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

using namespace std;
namespace fs = boost::filesystem;

void ConfigParser::parse(const string &filename, Plankton &plankton) {
    logger.info("Parsing configuration file " + filename);
    this->_config = make_unique<toml::table>(toml::parse_file(filename));
    this->parse_network(plankton._network);
    this->parse_openflow(plankton._openflow, plankton._network);
    this->parse_policies(plankton._policies, plankton._network);
}

void ConfigParser::parse_network(Network &network) {
    auto nodes_config = this->_config->get_as<toml::array>("nodes");

    if (nodes_config) {
        for (const auto &cfg : *nodes_config) {
            const toml::table &tbl = *cfg.as_table();
            Node *node = nullptr;
            auto type = tbl.get_as<string>("type");
            auto driver = tbl.get_as<string>("driver");

            if (!type || **type == "model") {
                node = new Node();
                this->parse_node(*node, tbl);
            } else if (**type == "emulation") {
                if (!driver || **driver == "docker") {
                    node = new DockerNode();
                    this->parse_dockernode(*static_cast<DockerNode *>(node),
                                           tbl);
                } else {
                    logger.error("Unknown driver: " + **driver);
                }

                auto mb = static_cast<Middlebox *>(node);
                mb->_driver = **driver;
                network.add_middlebox(mb);
            } else {
                logger.error("Unknown node type: " + **type);
            }

            network.add_node(node);
        }
    }

    logger.info("Loaded " + to_string(network.get_nodes().size()) + " nodes");
    auto links_config = this->_config->get_as<toml::array>("links");

    if (links_config) {
        for (const auto &cfg : *links_config) {
            const toml::table &tbl = *cfg.as_table();
            Link *link = new Link();
            this->parse_link(*link, tbl, network.get_nodes());
            network.add_link(link);
        }
    }

    logger.info("Loaded " + to_string(network.get_links().size()) + " links");

    // Populate L2 LANs (assuming there is no VLAN for now)
    for (const auto &[_, node] : network.get_nodes()) {
        for (Interface *intf : node->get_intfs_l2()) {
            if (!node->mapped_to_l2lan(intf)) {
                network.grow_and_set_l2_lan(node, intf);
            }
        }
    }
}

void ConfigParser::parse_node(Node &node, const toml::table &config) {
    auto node_name = config.get_as<string>("name");
    auto interfaces = config.get_as<toml::array>("interfaces");
    auto static_routes = config.get_as<toml::array>("static_routes");
    auto installed_routes = config.get_as<toml::array>("installed_routes");

    if (!node_name) {
        logger.error("Missing node name");
    }

    node.name = **node_name;

    if (interfaces) {
        for (const auto &cfg : *interfaces) {
            Interface *interface = new Interface();
            this->parse_interface(*interface, *cfg.as_table());
            node.add_interface(interface);
        }
    }

    if (static_routes) {
        for (const auto &cfg : *static_routes) {
            Route route;
            this->parse_route(route, *cfg.as_table());
            if (route.get_adm_dist() == 255) { // user did not specify adm dist
                route.adm_dist = 1;
            }
            node.rib.insert(std::move(route));
        }
    }

    if (installed_routes) {
        for (const auto &cfg : *installed_routes) {
            Route route;
            this->parse_route(route, *cfg.as_table());
            node.rib.insert(std::move(route));
        }
    }
}

void ConfigParser::parse_interface(Interface &interface,
                                   const toml::table &config) {
    auto intf_name = config.get_as<string>("name");
    auto ipv4_addr = config.get_as<string>("ipv4");

    if (!intf_name) {
        logger.error("Missing interface name");
    }

    interface.name = **intf_name;

    if (ipv4_addr) {
        interface.ipv4 = **ipv4_addr;
        interface.is_switchport = false;
    } else {
        interface.is_switchport = true;
    }
}

void ConfigParser::parse_route(Route &route, const toml::table &config) {
    auto network = config.get_as<string>("network");
    auto next_hop = config.get_as<string>("next_hop");
    auto interface = config.get_as<string>("interface");
    auto adm_dist = config.get_as<int64_t>("adm_dist");

    if (!network) {
        logger.error("Missing network");
    }
    if (!next_hop && !interface) {
        logger.error("Missing next hop IP address and interface");
    }

    route.network = IPNetwork<IPv4Address>(**network);
    if (next_hop) {
        route.next_hop = IPv4Address(**next_hop);
    }
    if (interface) {
        route.egress_intf = string(**interface);
    }
    if (adm_dist) {
        if (**adm_dist < 1 || **adm_dist > 254) {
            logger.error("Invalid administrative distance: " +
                         to_string(**adm_dist));
        }
        route.adm_dist = **adm_dist;
    }
}

void ConfigParser::parse_dockernode(DockerNode &dn, const toml::table &config) {
    this->parse_middlebox(dn, config);

    auto daemon = config.get_as<string>("docker-daemon");
    auto cntr_cfg = config.get_as<toml::table>("container");

    if (!cntr_cfg) {
        logger.error("Missing container configuration");
    }

    auto image = cntr_cfg->get_as<string>("image");
    auto working_dir = cntr_cfg->get_as<string>("workingDir");
    auto command = cntr_cfg->get_as<toml::array>("command");
    auto args = cntr_cfg->get_as<toml::array>("args");
    auto cfg_files = cntr_cfg->get_as<toml::array>("config_files");
    auto ports = cntr_cfg->get_as<toml::array>("ports");
    auto env = cntr_cfg->get_as<toml::array>("env");
    auto mounts = cntr_cfg->get_as<toml::array>("volumeMounts");
    auto sysctls = cntr_cfg->get_as<toml::array>("sysctls");

    if (!daemon) {
        dn._daemon = "/var/run/docker.sock";
    } else {
        dn._daemon = **daemon;
    }

    if (image) {
        dn._image = **image;

        if (dn._image.find(':') == string::npos) {
            dn._image += ":latest";
        }
    } else {
        logger.error("Missing container image");
    }

    if (working_dir) {
        dn._working_dir = **working_dir;
    } else {
        logger.error("Missing container workingDir");
    }

    if (command) {
        for (const auto &cmd : *command) {
            if (!cmd.as_string()) {
                logger.error("Only strings are allowed");
            }

            dn._cmd.emplace_back(**cmd.as_string());
        }
    }

    if (args) {
        for (const auto &arg : *args) {
            if (!arg.as_string()) {
                logger.error("Only strings are allowed");
            }

            dn._cmd.emplace_back(**arg.as_string());
        }
    }

    if (ports) {
        for (const auto &port_config : *ports) {
            const auto &cfg = *port_config.as_table();
            auto port = cfg.get_as<int64_t>("containerPort");
            auto protocol = cfg.get_as<string>("protocol");
            proto proto_enum;

            if (!port) {
                logger.error("Missing containerPort");
            } else if (**port < 1) {
                logger.error("Invalid containerPort: " + to_string(**port));
            }

            if (!protocol) {
                logger.error("Missing protocol");
            } else if (**protocol == "tcp") {
                proto_enum = proto::tcp;
            } else if (**protocol == "udp") {
                proto_enum = proto::udp;
            } else {
                logger.error("Unsupported protocol: " + **protocol);
            }

            dn._ports.emplace_back(proto_enum, **port);
        }
    }

    if (env) {
        for (const auto &env_config : *env) {
            const auto &cfg = *env_config.as_table();
            auto name = cfg.get_as<string>("name");
            auto value = cfg.get_as<string>("value");

            if (!name) {
                logger.error("Missing name");
            }

            if (!value) {
                logger.error("Missing value");
            }

            dn._env_vars.emplace(**name, **value);
        }
    }

    if (mounts) {
        for (const auto &mnt_config : *mounts) {
            const auto &cfg = *mnt_config.as_table();
            auto mount_path = cfg.get_as<string>("mountPath");
            auto host_path = cfg.get_as<string>("hostPath");
            auto driver = cfg.get_as<string>("driver");
            auto read_only = cfg.get_as<bool>("readOnly");

            if (!mount_path) {
                logger.error("Missing mountPath");
            }

            if (!host_path) {
                logger.error("Missing hostPath");
            }

            if (driver && **driver != "local") {
                logger.error("Unsupported volumnMounts driver: " + **driver);
            }

            struct DockerVolumeMount mount;
            mount.mount_path = **mount_path;
            mount.host_path = **host_path;
            mount.driver = driver ? **driver : "local";
            mount.read_only = read_only ? **read_only : false;
            dn._mounts.emplace_back(std::move(mount));
        }
    }

    if (sysctls) {
        for (const auto &sysctl_config : *sysctls) {
            const auto &cfg = *sysctl_config.as_table();
            auto key = cfg.get_as<string>("key");
            auto value = cfg.get_as<string>("value");

            if (!key) {
                logger.error("Missing key");
            }

            if (!value) {
                logger.error("Missing value");
            }

            dn._sysctls.emplace(**key, **value);
        }
    }

    // Pull the image
    DockerAPI dapi(dn.daemon());
    auto img_json = dapi.pull(dn.image());

    // If no command is specified, use the ENTRYPOINT and CMD from the image.
    if (dn._cmd.empty()) {
        auto &entrypoint = img_json["data"]["Config"]["Entrypoint"];
        auto &cmd = img_json["data"]["Config"]["Cmd"];

        if (entrypoint.IsArray()) {
            for (const auto &arg : entrypoint.GetArray()) {
                string arg_str(arg.GetString());
                if (!arg_str.empty()) {
                    dn._cmd.emplace_back(std::move(arg_str));
                }
            }
        }

        if (cmd.IsArray()) {
            for (const auto &arg : cmd.GetArray()) {
                string arg_str(arg.GetString());
                if (!arg_str.empty()) {
                    dn._cmd.emplace_back(std::move(arg_str));
                }
            }
        }
    }

    // Search for config file locations and parse for IP and ports
    if (cfg_files) {
        Docker docker(&dn, false);
        docker.init();
        docker.enterns();

        // Inside the container namespaces
        for (const auto &cfg_file : *cfg_files) {
            if (!cfg_file.as_string()) {
                logger.error("Only strings are allowed");
            }

            const string filename = **cfg_file.as_string();
            ifstream ifs(filename);

            if (!ifs) {
                logger.error("Failed to open " + filename);
            }

            stringstream buffer;
            buffer << ifs.rdbuf();
            const string config = buffer.str();
            this->parse_config_string(dn, config);
        }

        docker.leavens();
    }

    // Parse command, exposed ports, and envs for interesting IP/ports
    for (size_t i = 1; i < dn.cmd().size(); ++i) {
        this->parse_config_string(dn, dn.cmd()[i]);
    }

    for (const auto &[_, port] : dn.ports()) {
        dn._ec_ports.insert(port);
    }

    for (const auto &[_, value] : dn.env_vars()) {
        this->parse_config_string(dn, value);
    }
}

void ConfigParser::parse_middlebox(Middlebox &middlebox,
                                   const toml::table &config) {
    this->parse_node(middlebox, config);

    // TODO:
    // After eBPF drop tracing is implemented, we don't need to estimate the
    // latency when it's enabled
    if (!this->_has_lat_estimate) {
        this->estimate_pkt_lat();
    }

    middlebox._lat_avg = this->_lat_avg;
    middlebox._lat_mdev = this->_lat_mdev;
}

void ConfigParser::estimate_pkt_lat() {
    // TODO: rewrite
    /**
     * [192.168.1.2/24]                        [192.168.2.2/24]
     * (node1)-------------------(mb)-------------------(node2)
     *            [192.168.1.1/24]  [192.168.2.1/24]
     */
    // Network network;

    // Middlebox *mb = new Middlebox(false);
    // mb->name = "mb";
    // Interface *mb_eth0 = new Interface();
    // mb_eth0->name = "eth0";
    // mb_eth0->ipv4 = "192.168.1.1/24";
    // mb_eth0->is_switchport = false;
    // Interface *mb_eth1 = new Interface();
    // mb_eth1->name = "eth1";
    // mb_eth1->ipv4 = "192.168.2.1/24";
    // mb_eth1->is_switchport = false;
    // mb->add_interface(mb_eth0);
    // mb->add_interface(mb_eth1);
    // mb->env = "netns";
    // NetFilter *nf = new NetFilter();
    // mb->app = nf;
    // nf->rp_filter = 0;
    // nf->rules = "*filter\n"
    //             ":INPUT ACCEPT [0:0]\n"
    //             ":FORWARD ACCEPT [0:0]\n"
    //             ":OUTPUT ACCEPT [0:0]\n"
    //             "COMMIT\n";
    // Config::parse_appliance_config(nf, nf->rules);
    // network.add_middlebox(mb);
    // network.add_node(mb);

    // Node *node1 = new Node(), *node2 = new Node();
    // node1->name = "node1";
    // node2->name = "node2";
    // Interface *node1_eth0 = new Interface();
    // node1_eth0->name = "eth0";
    // node1_eth0->ipv4 = "192.168.1.2/24";
    // node1_eth0->is_switchport = false;
    // Interface *node2_eth0 = new Interface();
    // node2_eth0->name = "eth0";
    // node2_eth0->ipv4 = "192.168.2.2/24";
    // node2_eth0->is_switchport = false;
    // node1->add_interface(node1_eth0);
    // node2->add_interface(node2_eth0);
    // network.add_node(node1);
    // network.add_node(node2);

    // Link *link = new Link();
    // link->node1 = mb;
    // link->node2 = node1;
    // link->intf1 = mb_eth0;
    // link->intf2 = node1_eth0;
    // if (link->node1 > link->node2) {
    //     swap(link->node1, link->node2);
    //     swap(link->intf1, link->intf2);
    // }
    // network.add_link(link);
    // link = new Link();
    // link->node1 = mb;
    // link->node2 = node2;
    // link->intf1 = mb_eth1;
    // link->intf2 = node2_eth0;
    // if (link->node1 > link->node2) {
    //     swap(link->node1, link->node2);
    //     swap(link->intf1, link->intf2);
    // }
    // network.add_link(link);

    // // register signal handler
    // struct sigaction action, *oldaction = nullptr;
    // action.sa_handler = [](int) {};
    // sigemptyset(&action.sa_mask);
    // sigaddset(&action.sa_mask, SIGUSR1);
    // action.sa_flags = SA_NOCLDSTOP;
    // sigaction(SIGUSR1, &action, oldaction);

    // Emulation emulation;
    // emulation.init(mb);
    // mb->emulation = &emulation;

    // // inject packets
    // Packet packet(mb_eth0, "192.168.1.2", "192.168.2.2", 49152, 80, 0, 0,
    //               PS_TCP_INIT_1);
    // assert(emulation._dropmon == false);
    // emulation._dropmon = true; // temporarily disable timeout
    // for (int i = 0; i < 10; ++i) {
    //     emulation.send_pkt(packet);
    //     packet.set_src_port(packet.get_src_port() + 1);
    // }
    // emulation._dropmon = false;

    // // calculate latency average and mean deviation
    // const vector<pair<uint64_t, uint64_t>> &pkt_latencies =
    //     Stats::get_pkt_latencies();
    // long long avg = 0;
    // for (const auto &lat : pkt_latencies) {
    //     avg += lat.second / 1000 + 1;
    // }
    // avg /= pkt_latencies.size();
    // Config::latency_avg = chrono::microseconds(avg);
    // long long mdev = 0;
    // for (const auto &lat : pkt_latencies) {
    //     mdev += abs((long long)(lat.second / 1000 + 1) - avg);
    // }
    // mdev /= pkt_latencies.size();
    // Config::latency_mdev = chrono::microseconds(mdev);

    // // reset signal handler
    // sigaction(SIGUSR1, oldaction, nullptr);

    // emulation.teardown();
    // Stats::clear_latencies();
    // Config::got_latency_estimate = true;
}

#define IPV4_PREF_REGEX "\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}/\\d+\\b"
#define IPV4_ADDR_REGEX                                                        \
    "\\b(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})(?:[^/]|$)"
#define PORT_REGEX                                                             \
    "(?:port\\s+|\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:)(\\d+)\\b"

static const regex ip_prefix_regex(IPV4_PREF_REGEX);
static const regex ip_addr_regex(IPV4_ADDR_REGEX);
static const regex port_regex(PORT_REGEX);

void ConfigParser::parse_config_string(Middlebox &mb, const string &config) {
    smatch match;
    string subject;

    // search for IP prefix patterns
    subject = config;
    while (regex_search(subject, match, ip_prefix_regex)) {
        mb._ec_ip_prefixes.emplace(match.str(0));
        subject = match.suffix();
    }

    // search for IP address patterns
    subject = config;
    while (regex_search(subject, match, ip_addr_regex)) {
        mb._ec_ip_addrs.emplace(match.str(1));
        subject = match.suffix();
    }

    // search for port patterns
    subject = config;
    while (regex_search(subject, match, port_regex)) {
        mb._ec_ports.insert(stoul(match.str(1)));
        subject = match.suffix();
    }
}

void ConfigParser::parse_link(Link &link,
                              const toml::table &config,
                              const map<string, Node *> &nodes) {
    auto node1_name = config.get_as<string>("node1");
    auto intf1_name = config.get_as<string>("intf1");
    auto node2_name = config.get_as<string>("node2");
    auto intf2_name = config.get_as<string>("intf2");

    if (!node1_name) {
        logger.error("Missing node1");
    }
    if (!intf1_name) {
        logger.error("Missing intf1");
    }
    if (!node2_name) {
        logger.error("Missing node2");
    }
    if (!intf2_name) {
        logger.error("Missing intf2");
    }

    auto node1_entry = nodes.find(**node1_name);
    if (node1_entry == nodes.end()) {
        logger.error("Unknown node: " + **node1_name);
    }
    link.node1 = node1_entry->second;
    auto node2_entry = nodes.find(**node2_name);
    if (node2_entry == nodes.end()) {
        logger.error("Unknown node: " + **node2_name);
    }
    link.node2 = node2_entry->second;
    link.intf1 = link.node1->get_interface(**intf1_name);
    link.intf2 = link.node2->get_interface(**intf2_name);

    // normalize
    if (link.node1 > link.node2) {
        swap(link.node1, link.node2);
        swap(link.intf1, link.intf2);
    } else if (link.node1 == link.node2) {
        logger.error("Invalid link: " + link.to_string());
    }
}

void ConfigParser::parse_openflow(OpenflowProcess &openflow,
                                  const Network &network) {
    auto of_cfg = this->_config->get_as<toml::table>("openflow");

    if (!of_cfg) {
        return;
    }

    auto updates_cfg = of_cfg->get_as<toml::array>("updates");

    if (!updates_cfg) {
        return;
    }

    for (const auto &update_cfg : *updates_cfg) {
        Node *node;
        Route route;
        this->parse_openflow_update(node, route, *update_cfg.as_table(),
                                    network);
        openflow.add_update(node, std::move(route));
    }

    if (openflow.num_nodes() > 0) {
        logger.info("Loaded " + to_string(openflow.num_updates()) +
                    " openflow updates for " + to_string(openflow.num_nodes()) +
                    " nodes");
    }
}

void ConfigParser::parse_openflow_update(Node *&node,
                                         Route &route,
                                         const toml::table &config,
                                         const Network &network) {
    auto node_name = config.get_as<string>("node");
    auto network_cidr = config.get_as<string>("network");
    auto outport = config.get_as<string>("outport");

    if (!node_name) {
        logger.error("Missing node name");
    }
    if (!network_cidr) {
        logger.error("Missing network");
    }
    if (!outport) {
        logger.error("Missing outport");
    }

    auto node_itr = network.get_nodes().find(**node_name);
    if (node_itr == network.get_nodes().end()) {
        logger.error("Unknown node: " + **node_name);
    }

    node = node_itr->second;

    if (node->is_emulated()) {
        logger.error("Unsupported node type");
    }

    route.network = IPNetwork<IPv4Address>(**network_cidr);
    route.egress_intf = string(**outport);
}

void ConfigParser::parse_policies(vector<shared_ptr<Policy>> &policies,
                                  const Network &network) {
    auto pol_arr = this->_config->get_as<toml::array>("policies");

    if (pol_arr) {
        this->parse_policy_array(policies, false, *pol_arr, network);
    }

    if (policies.size() == 1) {
        logger.info("Loaded 1 policy");
    } else {
        logger.info("Loaded " + to_string(policies.size()) + " policies");
    }
}

void ConfigParser::parse_policy_array(vector<shared_ptr<Policy>> &policies,
                                      bool correlated,
                                      const toml::array &pol_arr,
                                      const Network &network) {
    for (const auto &cfg : pol_arr) {
        const auto &tbl = *cfg.as_table();
        shared_ptr<Policy> policy;

        auto type = tbl.get_as<string>("type");
        if (!type) {
            logger.error("Missing policy type");
        }

        // Since make_shared<> requires public constructor, we need to call new
        // explicitly and then construct the shared_ptr from it.
        if (**type == "reachability") {
            auto pol = shared_ptr<ReachabilityPolicy>(
                new ReachabilityPolicy(correlated));
            this->parse_reachability(pol, tbl, network);
            policy = pol;
        } else if (*type == "reply-reachability") {
            auto pol = shared_ptr<ReplyReachabilityPolicy>(
                new ReplyReachabilityPolicy(correlated));
            this->parse_replyreachability(pol, tbl, network);
            policy = pol;
        } else if (*type == "waypoint") {
            auto pol =
                shared_ptr<WaypointPolicy>(new WaypointPolicy(correlated));
            this->parse_waypoint(pol, tbl, network);
            policy = pol;
        } else if (!correlated && *type == "one-request") {
            auto pol = shared_ptr<OneRequestPolicy>(new OneRequestPolicy());
            this->parse_onerequest(pol, tbl, network);
            policy = pol;
        } else if (!correlated && *type == "loadbalance") {
            auto pol = shared_ptr<LoadBalancePolicy>(new LoadBalancePolicy());
            this->parse_loadbalance(pol, tbl, network);
            policy = pol;
        } else if (!correlated && *type == "conditional") {
            auto pol = shared_ptr<ConditionalPolicy>(new ConditionalPolicy());
            this->parse_conditional(pol, tbl, network);
            policy = pol;
        } else if (!correlated && *type == "consistency") {
            auto pol = shared_ptr<ConsistencyPolicy>(new ConsistencyPolicy());
            this->parse_consistency(pol, tbl, network);
            policy = pol;
        } else {
            logger.error("Unknown policy type: " + **type);
        }

        assert(policy->conn_specs.size() <= MAX_CONNS);
        policies.push_back(std::move(policy));
    }
}

void ConfigParser::parse_reachability(shared_ptr<ReachabilityPolicy> &policy,
                                      const toml::table &config,
                                      const Network &network) {
    auto target_node_regex = config.get_as<string>("target_node");
    auto reachable = config.get_as<bool>("reachable");

    if (!target_node_regex) {
        logger.error("Missing target_node");
    }
    if (!reachable) {
        logger.error("Missing reachable");
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = **reachable;
    this->parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void ConfigParser::parse_replyreachability(
    shared_ptr<ReplyReachabilityPolicy> &policy,
    const toml::table &config,
    const Network &network) {
    auto target_node_regex = config.get_as<string>("target_node");
    auto reachable = config.get_as<bool>("reachable");

    if (!target_node_regex) {
        logger.error("Missing target_node");
    }
    if (!reachable) {
        logger.error("Missing reachable");
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->reachable = **reachable;
    this->parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void ConfigParser::parse_waypoint(shared_ptr<WaypointPolicy> &policy,
                                  const toml::table &config,
                                  const Network &network) {
    auto target_node_regex = config.get_as<string>("target_node");
    auto pass_through = config.get_as<bool>("pass_through");

    if (!target_node_regex) {
        logger.error("Missing target_node");
    }
    if (!pass_through) {
        logger.error("Missing pass_through");
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->pass_through = **pass_through;
    this->parse_connections(policy, config, network);
    assert(policy->conn_specs.size() == 1);
}

void ConfigParser::parse_onerequest(shared_ptr<OneRequestPolicy> &policy,
                                    const toml::table &config,
                                    const Network &network) {
    auto target_node_regex = config.get_as<string>("target_node");

    if (!target_node_regex) {
        logger.error("Missing target_node");
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    this->parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
}

void ConfigParser::parse_loadbalance(shared_ptr<LoadBalancePolicy> &policy,
                                     const toml::table &config,
                                     const Network &network) {
    auto target_node_regex = config.get_as<string>("target_node");
    auto max_vmr = config.get_as<double>("max_dispersion_index");

    if (!target_node_regex) {
        logger.error("Missing target_node");
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**target_node_regex))) {
            policy->target_nodes.insert(node.second);
        }
    }
    policy->max_dispersion_index = max_vmr ? **max_vmr : 1;
    this->parse_connections(policy, config, network);
    assert(policy->conn_specs.size() >= 1);
}

void ConfigParser::parse_conditional(shared_ptr<ConditionalPolicy> &policy,
                                     const toml::table &config,
                                     const Network &network) {
    parse_correlated_policies(policy, config, network);
}

void ConfigParser::parse_consistency(shared_ptr<ConsistencyPolicy> &policy,
                                     const toml::table &config,
                                     const Network &network) {
    parse_correlated_policies(policy, config, network);
}

void ConfigParser::parse_correlated_policies(std::shared_ptr<Policy> policy,
                                             const toml::table &config,
                                             const Network &network) {
    auto policies_config = config.get_as<toml::array>("correlated_policies");

    if (!policies_config) {
        logger.error("Missing correlated policies");
    }

    this->parse_policy_array(policy->correlated_policies, true,
                             *policies_config, network);
}

void ConfigParser::parse_connections(shared_ptr<Policy> policy,
                                     const toml::table &config,
                                     const Network &network) {
    auto conns_cfg = config.get_as<toml::array>("connections");

    if (!conns_cfg) {
        logger.error("Missing connections");
    }

    for (const auto &conn_cfg : *conns_cfg) {
        ConnSpec conn;
        this->parse_conn_spec(conn, *conn_cfg.as_table(), network);
        policy->conn_specs.emplace_back(std::move(conn));
    }
}

void ConfigParser::parse_conn_spec(ConnSpec &conn_spec,
                                   const toml::table &config,
                                   const Network &network) {
    auto proto_str = config.get_as<string>("protocol");
    auto src_node_regex = config.get_as<string>("src_node");
    auto dst_ip_str = config.get_as<string>("dst_ip");
    auto src_port = config.get_as<int64_t>("src_port");
    auto dst_ports = config.get_as<toml::array>("dst_port");
    auto owned_dst_only = config.get_as<bool>("owned_dst_only");

    if (!proto_str) {
        logger.error("Missing protocol");
    }
    if (!src_node_regex) {
        logger.error("Missing src_node");
    }
    if (!dst_ip_str) {
        logger.error("Missing dst_ip");
    }

    string proto_s = **proto_str;
    transform(proto_s.begin(), proto_s.end(), proto_s.begin(),
              [](unsigned char c) { return tolower(c); });
    if (proto_s == "tcp") {
        conn_spec.protocol = proto::tcp;
    } else if (proto_s == "udp") {
        conn_spec.protocol = proto::udp;
    } else if (proto_s == "icmp-echo") {
        conn_spec.protocol = proto::icmp_echo;
    } else {
        logger.error("Unknown protocol: " + **proto_str);
    }

    for (const auto &node : network.get_nodes()) {
        if (regex_match(node.first, regex(**src_node_regex))) {
            conn_spec.src_nodes.insert(node.second);
        }
    }

    string ip_str = **dst_ip_str;
    if (ip_str.find('/') == string::npos) {
        ip_str += "/32";
    }
    conn_spec.dst_ip = IPRange<IPv4Address>(ip_str);

    if (conn_spec.protocol == proto::tcp || conn_spec.protocol == proto::udp) {
        conn_spec.src_port = src_port ? **src_port : DYNAMIC_PORT;
        if (dst_ports) {
            for (const auto &dst_port : *dst_ports) {
                conn_spec.dst_ports.insert(**dst_port.as_integer());
            }
        }
    }

    conn_spec.owned_dst_only = owned_dst_only ? **owned_dst_only : false;
}
