#include "lib/net.hpp"

#include <cassert>
#include <net/if.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ios>
#include <libnet.h>

#include "packet.hpp"
#include "payload.hpp"
#include "pktbuffer.hpp"
#include "lib/logger.hpp"

Net::Net()
{
    char errbuf[LIBNET_ERRBUF_SIZE];
    l = libnet_init(LIBNET_LINK_ADV, NULL, errbuf);
    if (!l) {
        Logger::error(std::string("libnet_init() failed: ") + errbuf);
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
        case PS_TCP_L7_REQ_A:
        case PS_TCP_L7_REP_A:
        case PS_TCP_TERM_3:
            ctrl_flags = TH_ACK;
            break;
        case PS_TCP_L7_REQ:
        case PS_TCP_L7_REP:
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
        Logger::error(std::string("Can't build TCP header: ") + libnet_geterror(l));
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
        Logger::error(std::string("Can't build IP header: ") + libnet_geterror(l));
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
        Logger::error(std::string("Can't build ethernet header: ") + libnet_geterror(l));
    }
}

void Net::build_udp(const Packet& pkt, const uint8_t *src_mac,
                    const uint8_t *dst_mac) const
{
    libnet_ptag_t tag;
    const uint8_t *payload = pkt.get_payload()->get();
    uint32_t payload_size = pkt.get_payload()->get_size();

    tag = libnet_build_udp(
              pkt.get_src_port(),           // source port
              pkt.get_dst_port(),           // destination port
              LIBNET_UDP_H + payload_size,  // length
              0,                            // checksum (0: autofill)
              payload,                      // payload
              payload_size,                 // payload length
              l,                            // libnet context
              0);                           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build UDP header: ") + libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
              LIBNET_IPV4_H + LIBNET_UDP_H + payload_size,  // length
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
        Logger::error(std::string("Can't build IP header: ") + libnet_geterror(l));
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
        Logger::error(std::string("Can't build ethernet header: ") + libnet_geterror(l));
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
        Logger::error("pkt_state is not related to ICMP echo");
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
        Logger::error(std::string("Can't build TCP header: ") + libnet_geterror(l));
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
        Logger::error(std::string("Can't build IP header: ") + libnet_geterror(l));
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
        Logger::error(std::string("Can't build ethernet header: ") + libnet_geterror(l));
    }
}

void Net::serialize(uint8_t **buffer, uint32_t *buffer_size, const Packet& pkt,
                    const uint8_t *src_mac, const uint8_t *dst_mac) const
{
    int pkt_state = pkt.get_pkt_state();

    if (PS_IS_TCP(pkt_state)) {
        build_tcp(pkt, src_mac, dst_mac);
    } else if (PS_IS_UDP(pkt_state)) {
        build_udp(pkt, src_mac, dst_mac);
    } else if (PS_IS_ICMP_ECHO(pkt_state)) {
        build_icmp_echo(pkt, src_mac, dst_mac);
    } else {
        Logger::error("Unsupported packet state " + std::to_string(pkt.get_pkt_state()));
    }

    if (libnet_adv_cull_packet(l, buffer, buffer_size) < 0) {
        Logger::error(std::string("libnet_adv_cull_packet() failed: ")
                      + libnet_geterror(l));
    }

    libnet_clear_packet(l);
}

void Net::free(uint8_t *buffer) const
{
    libnet_adv_free_packet(l, buffer);
}

void Net::deserialize(Packet& pkt, const PktBuffer& pb) const
{
    const uint8_t *buffer = pb.get_buffer();

    // filter out irrelevant frames
    const uint8_t *dst_mac = buffer;
    uint8_t id_mac[6] = ID_ETH_ADDR;
    if (memcmp(dst_mac, id_mac, 6) != 0) {
        goto bad_packet;
    }

    uint16_t ethertype;
    memcpy(&ethertype, buffer + 12, 2);
    ethertype = ntohs(ethertype);
    if (ethertype == ETHERTYPE_IP) {    // IPv4 packets (buffer + 14)
        // source and destination IP addresses
        uint32_t src_ip, dst_ip;
        memcpy(&src_ip, buffer + 26, 4);
        memcpy(&dst_ip, buffer + 30, 4);
        src_ip = ntohl(src_ip);
        dst_ip = ntohl(dst_ip);
        pkt.set_src_ip(src_ip);
        pkt.set_dst_ip(dst_ip);

        uint8_t ip_proto;
        memcpy(&ip_proto, buffer + 23, 1);
        if (ip_proto == IPPROTO_TCP) {          // TCP packets
            // source and destination TCP port
            uint16_t src_port, dst_port;
            memcpy(&src_port, buffer + 34, 2);
            memcpy(&dst_port, buffer + 36, 2);
            src_port = ntohs(src_port);
            dst_port = ntohs(dst_port);
            pkt.set_src_port(src_port);
            pkt.set_dst_port(dst_port);
            // seq and ack numbers
            uint32_t seq, ack;
            memcpy(&seq, buffer + 38, 4);
            memcpy(&ack, buffer + 42, 4);
            seq = ntohl(seq);
            ack = ntohl(ack);
            pkt.set_seq_no(seq);
            pkt.set_ack_no(ack);
            // TCP flags
            uint8_t flags;
            memcpy(&flags, buffer + 47, 1);
            pkt.set_pkt_state(flags | 0x80U);
            /*
             * NOTE:
             * Store the TCP flags in pkt_state for now, which will be converted
             * to the real pkt_state in ForwardingProcess (calling
             * Net::convert_pkt_state), because the knowledge of the current
             * connection state is required to interprete the pkt_state. The
             * highest bit of the variable is used to indicate unconverted TCP
             * flags.
             */
        } else if (ip_proto == IPPROTO_UDP) {   // UDP packets
            // source and destination TCP port
            uint16_t src_port, dst_port;
            memcpy(&src_port, buffer + 34, 2);
            memcpy(&dst_port, buffer + 36, 2);
            src_port = ntohs(src_port);
            dst_port = ntohs(dst_port);
            pkt.set_src_port(src_port);
            pkt.set_dst_port(dst_port);
        } else if (ip_proto == IPPROTO_ICMP) {  // ICMP packets
            // ICMP type
            uint8_t icmp_type;
            memcpy(&icmp_type, buffer + 34, 1);
            if (icmp_type == ICMP_ECHO) {
                pkt.set_pkt_state(PS_ICMP_ECHO_REQ);
            } else if (icmp_type == ICMP_ECHOREPLY) {
                pkt.set_pkt_state(PS_ICMP_ECHO_REP);
            } else {
                Logger::warn("ICMP type: " + std::to_string(icmp_type));
                goto bad_packet;
            }
        } else {    // unsupported L4 (or L3.5) protocols
            goto bad_packet;
        }
    } else {    // unsupported L3 protocol
        goto bad_packet;
    }

    pkt.set_intf(pb.get_intf());
    return;

bad_packet:
    pkt.clear();
}

void Net::convert_pkt_state(
    Packet& pkt, uint8_t old_pkt_state, bool change_direction) const
{
    // the highest bit (0x80) is used to indicate unconverted TCP flags
    if (PS_IS_TCP(old_pkt_state) && pkt.get_pkt_state() & 0x80U) {
        uint8_t pkt_state = 0;
        uint8_t flags = pkt.get_pkt_state() & (~0x80U);
        if (flags == TH_SYN) {
            pkt_state = PS_TCP_INIT_1;
        } else if (flags == (TH_SYN | TH_ACK)) {
            pkt_state = PS_TCP_INIT_2;
        } else if (flags == TH_ACK) {
            switch (old_pkt_state) {
                case PS_TCP_INIT_2:
                case PS_TCP_L7_REQ:
                case PS_TCP_L7_REP:
                case PS_TCP_TERM_2:
                    pkt_state = old_pkt_state + 1;
                    break;
                case PS_TCP_INIT_3:
                case PS_TCP_L7_REQ_A:
                case PS_TCP_L7_REP_A:
                case PS_TCP_TERM_3:
                    pkt_state = old_pkt_state;
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_PUSH | TH_ACK)) {
            switch (old_pkt_state) {
                case PS_TCP_INIT_3:
                case PS_TCP_L7_REQ_A:
                    pkt_state = old_pkt_state + 1;
                    break;
                case PS_TCP_L7_REQ:
                case PS_TCP_L7_REP:
                    pkt_state = old_pkt_state;
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_FIN | TH_ACK)) {
            if (change_direction) {
                pkt_state = old_pkt_state + 1;
            } else {
                pkt_state = old_pkt_state;
            }
            switch (old_pkt_state) {
                case PS_TCP_L7_REP_A:
                case PS_TCP_TERM_1:
                case PS_TCP_TERM_2:
                case PS_TCP_TERM_3:
                    break;
                default:
                    Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else {
            Logger::error("Invalid TCP flags: " + std::to_string(flags));
        }
        pkt.set_pkt_state(pkt_state);
    } else if (PS_IS_UDP(old_pkt_state)) {
        uint8_t pkt_state;
        if (change_direction) {
            pkt_state = old_pkt_state + 1;
            if (!PS_SAME_PROTO(pkt_state, old_pkt_state)) {
                Logger::error("Change direction after receiving reply");
            }
        } else {
            pkt_state = old_pkt_state;
        }
        pkt.set_pkt_state(pkt_state);
    }
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

static void write_value_to_file(int val, const std::string& filename)
{
    int fd = open(filename.c_str(), O_WRONLY);
    if (fd < 0) {
        Logger::error("Failed to open " + filename, errno);
    }
    std::string val_s = std::to_string(val);
    if (write(fd, val_s.c_str(), val_s.size()) < 0) {
        Logger::error("Failed writting \"" + val_s + "\" to " + filename);
    }
    close(fd);
}

void Net::set_forwarding(int val) const
{
    write_value_to_file(val, "/proc/sys/net/ipv4/conf/all/forwarding");
}

void Net::set_rp_filter(int val) const
{
    // The max value from conf/{all,interface}/rp_filter is used when doing
    // source validation on the {interface}.

    // set conf.all.rp_filter
    write_value_to_file(val, "/proc/sys/net/ipv4/conf/all/rp_filter");

    // set conf.{interface}.rp_filter
    struct if_nameindex *if_nidxs = if_nameindex();
    assert(if_nidxs);
    for (struct if_nameindex *intf = if_nidxs; intf->if_name != NULL; intf++) {
        write_value_to_file(val, "/proc/sys/net/ipv4/conf/"
                            + std::string(intf->if_name) + "/rp_filter");
    }
    if_freenameindex(if_nidxs);
}
