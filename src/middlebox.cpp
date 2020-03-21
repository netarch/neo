#include "middlebox.hpp"

#include <libnet.h>

#include "mb-env/netns.hpp"
#include "mb-app/netfilter.hpp"
#include "mb-app/ipvs.hpp"
#include "mb-app/cache-proxy.hpp"
#include "stats.hpp"

Middlebox::Middlebox(const std::shared_ptr<cpptoml::table>& node_config)
    : Node(node_config), node_pkt_hist(nullptr), listener(nullptr),
      listener_end(false)
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
    } else if (*appliance == "ipvs") {
        app = new IPVS(node_config);
    } else if (*appliance == "squid-proxy") {
        app = new CacheProxy(node_config);
    } else {
        Logger::get().err("Unknown appliance: " + *appliance);
    }
}


Middlebox::~Middlebox()
{
    //if (listener) {
    //    listener_end = true;
    //    Packet dummy(intfs.begin()->second);
    //    env->inject_packet(dummy);
    //    if (listener->joinable()) {
    //        listener->join();
    //    }
    //}
    delete listener;
    delete app;
    delete env;
}

void Middlebox::listen_packets()
{
    std::list<PktBuffer> pkts;
    uint8_t id_mac[6] = ID_ETH_ADDR;

    while (!listener_end) {
        // read output packet (block if no packet)
        pkts = env->read_packets();

        for (PktBuffer pb : pkts) {
            uint8_t *buffer = pb.get_buffer();

            uint8_t *dst_mac = buffer;
            if (memcmp(dst_mac, id_mac, 6) != 0) {
                continue;
            }

            uint16_t ethertype;
            memcpy(&ethertype, buffer + 12, 2);
            ethertype = ntohs(ethertype);
            if (ethertype == ETHERTYPE_IP) {
                uint32_t dst_ip;
                memcpy(&dst_ip, buffer + 30, 4);
                dst_ip = ntohl(dst_ip);

                // TODO: save src_ip, dst_ip, egress interface, to construct a
                // Packet passed to the main thread to update packet related
                // states in Spin state vector
                // NOTE: not passing src_ip for now, unless it's need for source
                // NAT
                std::unique_lock<std::mutex> lck(mtx);
                recv_pkt.set_intf(pb.get_intf());
                recv_pkt.set_dst_ip(dst_ip);
                cv.notify_all();
                break;
            }
        }
    }
}

void Middlebox::init()
{
    env->init(*this);
    env->run(mb_app_init, app);
    if (!listener) {
        listener = new std::thread(&Middlebox::listen_packets, this);
    }
}

int Middlebox::rewind(NodePacketHistory *nph)
{
    if (node_pkt_hist == nph) {
        return -1;
    }

    int rewind_injections = 0;

    Logger::get().info("============== rewind starts ==============");

    // reset middlebox state
    env->run(mb_app_reset, app);

    // replay history
    if (nph) {
        rewind_injections = nph->get_packets().size();
        for (Packet *packet : nph->get_packets()) {
            send_pkt(*packet);
        }
    }
    node_pkt_hist = nph;

    Logger::get().info("==============  rewind ends  ==============");

    return rewind_injections;
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    node_pkt_hist = nph;
}

Packet Middlebox::send_pkt(const Packet& pkt)
{
    std::unique_lock<std::mutex> lck(mtx);
    recv_pkt.clear();

    // inject packet
    Logger::get().info("Injecting packet " + pkt.to_string());
    env->inject_packet(pkt);

    Stats::get().set_pkt_lat_t1();

    // wait for timeout
    std::cv_status status = cv.wait_for(lck, app->get_timeout());

    Stats::get().set_pkt_latency();

    // logging
    if (status == std::cv_status::timeout) {
        Logger::get().info("Timeout!");
    }

    // return the received packet
    return recv_pkt;
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
