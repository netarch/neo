#include "lib/net.hpp"

#include <string>

#include "lib/logger.hpp"

Net::Net()
{
    l = libnet_init(LIBNET_LINK_ADV, NULL, errbuf);
    if (!l) {
        Logger::get().err(std::string("libnet_init() failed: ") + errbuf);
    }
}

Net::~Net()
{
    libnet_destroy(l);
}

Net& Net::get()
{
    static Net instance;
    return instance;
}

uint32_t Net::get_pkt(uint8_t **packet,
                      const Packet& pkt,
                      // L7
                      const uint8_t *payload,
                      uint32_t payload_size,
                      // TCP
                      uint16_t src_port,
                      uint16_t dst_port,
                      uint32_t seq,
                      uint32_t ack,
                      uint8_t ctrl_flags,
                      // Ethernet
                      const uint8_t *src_mac,
                      const uint8_t *dst_mac)
{
    libnet_ptag_t tag;
    libnet_clear_packet(l);

    //tag = libnet_build_tcp_options(
    //        "\x00",
    //        1,
    //        l,
    //        0);
    //if (tag < 0) {
    //    Logger::get().err(std::string("Can't build TCP options: ")
    //                      + libnet_geterror(l));
    //}

    tag = libnet_build_tcp(
              src_port,
              dst_port,
              seq,
              ack,
              ctrl_flags,
              65535,
              0,
              0,
              LIBNET_TCP_H + payload_size,
              payload,
              payload_size,
              l,
              0);
    if (tag < 0) {
        Logger::get().err(std::string("Can't build TCP header: ")
                          + libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
              LIBNET_IPV4_H + LIBNET_TCP_H + payload_size,
              0,
              242,
              0,
              64,
              IPPROTO_TCP,
              0,
              htonl(pkt.get_src_ip().get_value()),
              htonl(pkt.get_dst_ip().get_value()),
              NULL,
              0,
              l,
              0);
    if (tag < 0) {
        Logger::get().err(std::string("Can't build IP header: ")
                          + libnet_geterror(l));
    }

    tag = libnet_build_ethernet(
              src_mac,
              dst_mac,
              ETHERTYPE_IP,
              NULL,
              0,
              l,
              0);
    if (tag < 0) {
        Logger::get().err(std::string("Can't build ethernet header: ")
                          + libnet_geterror(l));
    }

    uint32_t packet_size;

    if (libnet_adv_cull_packet(l, packet, &packet_size) < 0) {
        Logger::get().err(std::string("libnet_adv_cull_packet() failed: ")
                          + libnet_geterror(l));
    }

    return packet_size;
}

void Net::free_pkt(uint8_t *packet)
{
    libnet_adv_free_packet(l, packet);
}
