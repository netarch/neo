#include "middlebox.hpp"

#include <libnet.h>

#include "stats.hpp"
#include "lib/logger.hpp"

Middlebox::Middlebox()
    : env(nullptr), app(nullptr), node_pkt_hist(nullptr), listener(nullptr),
      listener_ended(false)
{
}

Middlebox::~Middlebox()
{
    //if (listener) {
    //    listener_ended = true;
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
    std::vector<Packet> pkts;

    while (!listener_ended) {
        // read the output packets (it will block if there is no packet)
        pkts = env->read_packets();

        if (!pkts.empty()) {
            std::unique_lock<std::mutex> lck(mtx);
            recv_pkts = pkts;
            cv.notify_all();
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
        Logger::info(this->get_name() + " up to date, no need to rewind");
        return -1;
    }

    int rewind_injections = 0;

    Logger::info("============== rewind starts (" + this->get_name() + ") ==============");

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

    Logger::info("==============  rewind ends  (" + this->get_name() + ") ==============");

    return rewind_injections;
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    node_pkt_hist = nph;
}

std::vector<Packet> Middlebox::send_pkt(const Packet& pkt)
{
    std::unique_lock<std::mutex> lck(mtx);
    recv_pkts.clear();

    // inject packet
    Logger::info("Injecting packet: " + pkt.to_string());
    env->inject_packet(pkt);

    Stats::set_pkt_lat_t1();

    // wait for timeout
    std::cv_status status = cv.wait_for(lck, app->get_timeout());

    Stats::set_pkt_latency();

    // logging
    if (status == std::cv_status::timeout) {
        Logger::info("Timeout!");
    }
    /*
     * NOTE:
     * We don't process the read packets in the critical section (i.e., here).
     * Instead, we process the read packets in ForwardingProcess (the caller),
     * which is also because the knowledge of the current connection state is
     * required to process it correctly, as mentioned in lib/net.cpp.
     */

    // return the received packets
    return recv_pkts;
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)),
    const RoutingTable *rib __attribute__((unused)),
    std::unordered_set<IPv4Address> *looked_up_ips __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
