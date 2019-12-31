#include "lib/net.hpp"

#include <string>

#include "payload.hpp"
#include "lib/logger.hpp"

Net::Net()
{
    char errbuf[LIBNET_ERRBUF_SIZE];
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

uint32_t Net::serialize(uint8_t **buffer, const Packet& pkt,
                        const uint8_t *src_mac,
                        const uint8_t *dst_mac) const
{
    libnet_ptag_t tag;
    libnet_clear_packet(l);

    uint8_t ctrl_flags = 0;
    switch (pkt.get_pkt_state()) {
        case PS_TCP_INIT_1:
            ctrl_flags = TH_SYN;
            break;
        case PS_TCP_INIT_2:
            ctrl_flags = TH_SYN | TH_ACK;
            break;
        case PS_TCP_INIT_3:
        case PS_HTTP_REQ_A:
        case PS_HTTP_REP_A:
        case PS_TCP_TERM_3:
            ctrl_flags = TH_ACK;
            break;
        case PS_HTTP_REQ:
        case PS_HTTP_REP:
            ctrl_flags = TH_PUSH | TH_ACK;
            break;
        case PS_TCP_TERM_1:
        case PS_TCP_TERM_2:
            ctrl_flags = TH_FIN | TH_ACK;
            break;
        default:
            Logger::get().err("Unknown packet state "
                              + std::to_string(pkt.get_pkt_state()));
    }
    const uint8_t *payload = pkt.get_payload()->get();
    uint32_t payload_size = pkt.get_payload()->get_size();

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
              pkt.get_src_port(),          // source port
              pkt.get_dst_port(),          // destination port
              pkt.get_seq(),               // sequence number
              pkt.get_ack(),               // acknowledgement number
              ctrl_flags,                   // control flags
              65535,                        // window size
              0,                            // checksum (0: autofill)
              0,                            // urgent pointer
              LIBNET_TCP_H + payload_size,  // length
              payload,                      // payload
              payload_size,                 // payload length
              l,                            // libnet context
              0);                           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::get().err(std::string("Can't build TCP header: ")
                          + libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
              LIBNET_IPV4_H + LIBNET_TCP_H + payload_size,  // length
              0,                                // ToS
              242,                              // identification number
              0,                                // fragmentation offset
              64,                               // TTL (time to live)
              IPPROTO_TCP,                      // upper layer protocol
              0,                                // checksum (0: autofill)
              pkt.get_src_ip().get_value(),     // source address
              pkt.get_dst_ip().get_value(),     // destination address
              NULL,                             // payload
              0,                                // payload length
              l,                                // libnet context
              0);                               // ptag (0: build a new one)
    if (tag < 0) {
        Logger::get().err(std::string("Can't build IP header: ")
                          + libnet_geterror(l));
    }

    tag = libnet_build_ethernet(
              dst_mac,          // destination ethernet address
              src_mac,          // source ethernet address
              ETHERTYPE_IP,     // upper layer protocol
              NULL,             // payload
              0,                // payload length
              l,                // libnet context
              0);               // ptag (0: build a new one)
    if (tag < 0) {
        Logger::get().err(std::string("Can't build ethernet header: ")
                          + libnet_geterror(l));
    }

    uint32_t buffer_size;

    if (libnet_adv_cull_packet(l, buffer, &buffer_size) < 0) {
        Logger::get().err(std::string("libnet_adv_cull_packet() failed: ")
                          + libnet_geterror(l));
    }

    return buffer_size;
}

void Net::free(uint8_t *buffer) const
{
    libnet_adv_free_packet(l, buffer);
}
