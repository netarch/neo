#include "lib/net.hpp"

#include <string>
#include <sstream>
#include <iomanip>
#include <ios>

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

void Net::build_tcp(const Packet& pkt, const uint8_t *src_mac,
                    const uint8_t *dst_mac) const
{
    libnet_ptag_t tag;

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
    }

    const uint8_t *payload = pkt.get_payload()->get();
    uint32_t payload_size = pkt.get_payload()->get_size();

    tag = libnet_build_tcp(
              pkt.get_src_port(),           // source port
              pkt.get_dst_port(),           // destination port
              pkt.get_seq(),                // sequence number
              pkt.get_ack(),                // acknowledgement number
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
              htonl(pkt.get_src_ip().get_value()),  // source address
              htonl(pkt.get_dst_ip().get_value()),  // destination address
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
}

void Net::build_icmp_echo(const Packet& pkt, const uint8_t *src_mac,
                          const uint8_t *dst_mac) const
{
    libnet_ptag_t tag;

    uint8_t icmp_type = 0;
    if (pkt.get_pkt_state() == PS_ICMP_ECHO_REQ) {
        icmp_type = ICMP_ECHO;
    } else if (pkt.get_pkt_state() == PS_ICMP_ECHO_REP) {
        icmp_type = ICMP_ECHOREPLY;
    } else {
        Logger::get().err("pkt_state is not related to ICMP echo");
    }

    tag = libnet_build_icmpv4_echo(
              icmp_type,    // ICMP type
              0,            // ICMP code
              0,            // checksum (0: autofill)
              42,           // identification number
              0,            // packet sequence number
              NULL,         // payload
              0,            // payload length
              l,            // libnet context
              0);           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::get().err(std::string("Can't build TCP header: ")
                          + libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
              LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H, // length
              0,                                    // ToS
              42,                                   // identification number
              0,                                    // fragmentation offset
              64,                                   // TTL (time to live)
              IPPROTO_TCP,                          // upper layer protocol
              0,                                    // checksum (0: autofill)
              htonl(pkt.get_src_ip().get_value()),  // source address
              htonl(pkt.get_dst_ip().get_value()),  // destination address
              NULL,                                 // payload
              0,                                    // payload length
              l,                                    // libnet context
              0);                                   // ptag (0: build a new one)
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
}

void Net::serialize(uint8_t **buffer, uint32_t *buffer_size, const Packet& pkt,
                    const uint8_t *src_mac, const uint8_t *dst_mac) const
{
    int pkt_state = pkt.get_pkt_state();

    if (PS_IS_TCP(pkt_state)) {
        build_tcp(pkt, src_mac, dst_mac);
    } else if (PS_IS_ICMP_ECHO(pkt_state)) {
        build_icmp_echo(pkt, src_mac, dst_mac);
    } else {
        Logger::get().err("Unsupported packet state "
                          + std::to_string(pkt.get_pkt_state()));
    }

    if (libnet_adv_cull_packet(l, buffer, buffer_size) < 0) {
        Logger::get().err(std::string("libnet_adv_cull_packet() failed: ")
                          + libnet_geterror(l));
    }

    libnet_clear_packet(l);
}

void Net::free(uint8_t *buffer) const
{
    libnet_adv_free_packet(l, buffer);
}

std::string Net::mac_to_str(const uint8_t *mac) const
{
    std::stringstream ss;

    ss << std::setfill('0') << std::setw(2) << std::hex << mac[0];
    for (int i = 1; i < 6; ++i) {
        ss << ":" << std::setfill('0') << std::setw(2) << std::hex << mac[i];
    }

    return ss.str();
}
