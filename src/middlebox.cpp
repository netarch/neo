#include "middlebox.hpp"
#include "mb-env/netns.hpp"
#include "mb-app/netfilter.hpp"

Middlebox::Middlebox(const std::shared_ptr<cpptoml::table>& node_config)
    : Node(node_config), node_pkt_hist(nullptr)
{
    auto environment = node_config->get_as<std::string>("env");
    auto appliance = node_config->get_as<std::string>("app");

    if (!environment) {
        Logger::get_instance().err("Missing environment");
    }
    if (!appliance) {
        Logger::get_instance().err("Missing appliance");
    }

    if (*environment == "netns") {
        env = new NetNS();
    } else {
        Logger::get_instance().err("Unknown environment: " + *environment);
    }

    if (*appliance == "netfilter") {
        app = new NetFilter(node_config);
    } else {
        Logger::get_instance().err("Unknown appliance: " + *appliance);
    }
}

Middlebox::~Middlebox()
{
    delete app;
    delete env;
}

void Middlebox::init()
{
    env->init(this);
    env->run(mb_app_init, app);
}

void Middlebox::rewind(NodePacketHistory *nph)
{
    env->run(mb_app_reset, app);

    // replay history (TODO)

    node_pkt_hist = nph;
}

NodePacketHistory *Middlebox::get_node_pkt_hist() const
{
    return node_pkt_hist;
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    node_pkt_hist = nph;
}

std::set<FIB_IPNH> Middlebox::send_pkt(const Packet& pkt __attribute__((unused)))
{
    // TODO
    // inject packet
    //env->inject_packet(interface, packet);
    // return next hop(s)
    return std::set<FIB_IPNH>();
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
