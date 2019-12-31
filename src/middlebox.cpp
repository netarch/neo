#include "middlebox.hpp"

#include "mb-env/netns.hpp"
#include "mb-app/netfilter.hpp"
#include "lib/net.hpp"

Middlebox::Middlebox(const std::shared_ptr<cpptoml::table>& node_config)
    : Node(node_config), node_pkt_hist(nullptr)
{
    auto environment = node_config->get_as<std::string>("env");
    auto appliance = node_config->get_as<std::string>("app");

    if (!environment) {
        Logger::get().err("Missing environment");
    }
    if (!appliance) {
        Logger::get().err("Missing appliance");
    }

    if (*environment == "netns") {
        env = new NetNS();
    } else {
        Logger::get().err("Unknown environment: " + *environment);
    }

    if (*appliance == "netfilter") {
        app = new NetFilter(node_config);
    } else {
        Logger::get().err("Unknown appliance: " + *appliance);
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
    if (node_pkt_hist == nph) {
        return;
    }

    env->run(mb_app_reset, app);

    // replay history (TODO)

    node_pkt_hist = nph;
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    node_pkt_hist = nph;
}

std::set<FIB_IPNH> Middlebox::send_pkt(const Packet& pkt)
{
    // serialize the packet
    uint8_t src_mac[6] = {0}, dst_mac[6] = {0};
    env->get_mac(pkt.get_intf(), dst_mac);
    uint8_t *buffer;
    uint32_t buffer_size = Net::get().serialize(&buffer, pkt, src_mac, dst_mac);
    // inject packet
    env->inject_packet(pkt.get_intf(), buffer, buffer_size);
    Net::get().free(buffer);

    // TODO: observe/read output packet
    // env->read_packet();
    // deserialize
    // find egress interface
    // find next hop interface and node

    // return next hop(s)
    return std::set<FIB_IPNH>();
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
