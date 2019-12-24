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

std::set<FIB_IPNH> Middlebox::send_pkt(const Packet& pkt)
{
    // inject packet (TODO: 3-way handshake)
    // construct libnet packet
    uint8_t *packet;
    uint8_t payload[] = "PLANKTON";
    uint32_t payload_size = sizeof(payload) / sizeof(uint8_t);
    uint8_t src_mac[6] = {0}, dst_mac[6] = {0};
    env->get_mac(pkt.get_intf(), dst_mac);
    uint32_t seq = 0, ack = 0;
    uint32_t packet_size
        = Net::get().get_pkt(
              &packet,          // concrete packet buffer
              pkt,              // pkt
              payload,          // payload
              payload_size,     // payload size
              //NULL,             // payload
              //0,                // payload size
              12345,            // source port
              80,               // destination port
              seq,              // sequence number
              ack,              // acknowledgement number
              TH_SYN,           // control flags
              src_mac,          // ethernet source
              dst_mac           // ethernet destination
          );
    env->inject_packet(pkt.get_intf(), packet, packet_size);
    Net::get().free_pkt(packet);

    // TODO: observe/read output packet
    // return next hop(s)
    return std::set<FIB_IPNH>();
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
