#include "middlebox.hpp"

#include <libnet.h>

#include "mb-env/netns.hpp"
#include "mb-app/netfilter.hpp"

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
    } else {
        Logger::get().err("Unknown appliance: " + *appliance);
    }
}

Middlebox::~Middlebox()
{
    if (listener && listener->joinable()) {
        listener_end = true;
        Packet dummy(intfs.begin()->second, (uint32_t)0, (uint32_t)0,
                     PS_ARP_REP);
        env->inject_packet(dummy);
        listener->join();
    }
    delete listener;
    delete app;
    delete env;
}

void Middlebox::listen_packets()
{
    std::list<PktBuffer> pkts;
    uint8_t id_mac[6] = ID_ETH_ADDR;

    while (!listener_end) {
        // read output packet
        pkts = env->read_packets();

        for (PktBuffer pb : pkts) {
            uint8_t *buffer = pb.get_buffer();
            uint8_t *dst_mac = buffer;
            uint16_t ethertype;
            memcpy(&ethertype, buffer + 12, 2);
            ethertype = ntohs(ethertype);

            if (ethertype == ETHERTYPE_ARP) {
                uint16_t arp_op;
                memcpy(&arp_op, buffer + 20, 2);
                arp_op = ntohs(arp_op);

                if (arp_op == ARPOP_REQUEST) {
                    uint32_t src_ip;
                    memcpy(&src_ip, buffer + 28, 4);
                    src_ip = ntohl(src_ip);
                    uint32_t target_ip;
                    memcpy(&target_ip, buffer + 38, 4);
                    target_ip = ntohl(target_ip);

                    Packet reply(pb.get_intf(), target_ip, src_ip, PS_ARP_REP);
                    env->inject_packet(reply);
                }
            } else if (ethertype == ETHERTYPE_IP) {
                if (memcmp(dst_mac, id_mac, 6) != 0) {
                    continue;
                }

                uint32_t dst_ip;
                memcpy(&dst_ip, buffer + 30, 4);
                dst_ip = ntohl(dst_ip);

                // find the next hop
                auto l2nh = get_peer(pb.get_intf()->get_name()); // L2 nhop
                if (l2nh.first) { // if the interface is truly connected
                    if (!l2nh.second->is_l2()) { // L2 nhop == L3 nhop
                        std::unique_lock<std::mutex> lck(mtx);
                        next_hops.insert(FIB_IPNH(l2nh.first, l2nh.second,
                                                  l2nh.first, l2nh.second));
                    } else {
                        L2_LAN *l2_lan = l2nh.first->get_l2lan(l2nh.second);
                        auto l3nh = l2_lan->find_l3_endpoint(dst_ip);
                        if (l3nh.first) {
                            std::unique_lock<std::mutex> lck(mtx);
                            next_hops.insert(FIB_IPNH(l3nh.first, l3nh.second,
                                                      l2nh.first, l2nh.second));
                        }
                    }
                }
            }
        }
        std::unique_lock<std::mutex> lck(mtx);
        if (!next_hops.empty()) {
            cv.notify_all();
        }
    }
}

void Middlebox::init()
{
    env->init(this);
    env->run(mb_app_init, app);
    if (!listener) {
        listener = new std::thread(&Middlebox::listen_packets, this);
    }
}

void Middlebox::rewind(NodePacketHistory *nph)
{
    if (node_pkt_hist == nph) {
        return;
    }

    // reset middlebox state
    env->run(mb_app_reset, app);

    // replay history
    if (nph) {
        for (Packet *packet : nph->get_packets()) {
            send_pkt(*packet);
        }
    }

    node_pkt_hist = nph;
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    node_pkt_hist = nph;
}

std::set<FIB_IPNH> Middlebox::send_pkt(const Packet& pkt)
{
    std::unique_lock<std::mutex> lck(mtx);
    next_hops.clear();

    // inject packet
    Logger::get().info("Injecting packet " + pkt.to_string());
    env->inject_packet(pkt);

    // wait for timeout
    std::chrono::milliseconds duration(50);
    cv.wait_for(lck, duration);

    // logging
    std::string nhops_str;
    nhops_str += to_string() + " -> [";
    for (auto nhop = next_hops.begin(); nhop != next_hops.end(); ++nhop) {
        nhops_str += " " + nhop->to_string();
    }
    nhops_str += " ]";
    Logger::get().info(nhops_str);

    // return next hop(s)
    return next_hops;
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
