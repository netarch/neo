#include "lib/net.hpp"

#include <cassert>
#include <fcntl.h>
#include <iomanip>
#include <ios>
#include <libnet.h>
#include <net/if.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/logger.hpp"
#include "packet.hpp"
#include "payload.hpp"
#include "pktbuffer.hpp"
#include "protocols.hpp"

Net::Net() {
    char errbuf[LIBNET_ERRBUF_SIZE];
    l = libnet_init(LIBNET_LINK_ADV, NULL, errbuf);
    if (!l) {
        Logger::error(std::string("libnet_init() failed: ") + errbuf);
    }
}

Net::~Net() {
    libnet_destroy(l);
}

Net &Net::get() {
    static Net instance;
    return instance;
}

void Net::build_tcp(const Packet &pkt,
                    const uint8_t *src_mac,
                    const uint8_t *dst_mac) const {
    libnet_ptag_t tag;

    uint8_t ctrl_flags = 0;
    switch (pkt.get_proto_state()) {
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

    Payload *pl = pkt.get_payload();
    const uint8_t *payload = pl ? pl->get() : nullptr;
    uint32_t payload_size = pl ? pl->get_size() : 0;

    tag = libnet_build_tcp(pkt.get_src_port(), // source port
                           pkt.get_dst_port(), // destination port
                           pkt.get_seq(),      // sequence number
                           pkt.get_ack(),      // acknowledgement number
                           ctrl_flags,         // control flags
                           65535,              // window size
                           0,                  // checksum (0: autofill)
                           0,                  // urgent pointer
                           LIBNET_TCP_H + payload_size, // length
                           payload,                     // payload
                           payload_size,                // payload length
                           l,                           // libnet context
                           0); // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build TCP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_TCP_H + payload_size, // length
        0,                                           // ToS
        242,                                         // identification number
        0,                                           // fragmentation offset
        64,                                          // TTL (time to live)
        IPPROTO_TCP,                                 // upper layer protocol
        0,                                           // checksum (0: autofill)
        htonl(pkt.get_src_ip().get_value()),         // source address
        htonl(pkt.get_dst_ip().get_value()),         // destination address
        NULL,                                        // payload
        0,                                           // payload length
        l,                                           // libnet context
        0); // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build IP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ethernet(dst_mac,      // destination ethernet address
                                src_mac,      // source ethernet address
                                ETHERTYPE_IP, // upper layer protocol
                                NULL,         // payload
                                0,            // payload length
                                l,            // libnet context
                                0);           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build ethernet header: ") +
                      libnet_geterror(l));
    }
}

void Net::build_udp(const Packet &pkt,
                    const uint8_t *src_mac,
                    const uint8_t *dst_mac) const {
    libnet_ptag_t tag;
    Payload *pl = pkt.get_payload();
    const uint8_t *payload = pl ? pl->get() : nullptr;
    uint32_t payload_size = pl ? pl->get_size() : 0;

    tag = libnet_build_udp(pkt.get_src_port(),          // source port
                           pkt.get_dst_port(),          // destination port
                           LIBNET_UDP_H + payload_size, // length
                           0,            // checksum (0: autofill)
                           payload,      // payload
                           payload_size, // payload length
                           l,            // libnet context
                           0);           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build UDP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_UDP_H + payload_size, // length
        0,                                           // ToS
        242,                                         // identification number
        0,                                           // fragmentation offset
        64,                                          // TTL (time to live)
        IPPROTO_UDP,                                 // upper layer protocol
        0,                                           // checksum (0: autofill)
        htonl(pkt.get_src_ip().get_value()),         // source address
        htonl(pkt.get_dst_ip().get_value()),         // destination address
        NULL,                                        // payload
        0,                                           // payload length
        l,                                           // libnet context
        0); // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build IP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ethernet(dst_mac,      // destination ethernet address
                                src_mac,      // source ethernet address
                                ETHERTYPE_IP, // upper layer protocol
                                NULL,         // payload
                                0,            // payload length
                                l,            // libnet context
                                0);           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build ethernet header: ") +
                      libnet_geterror(l));
    }
}

void Net::build_icmp_echo(const Packet &pkt,
                          const uint8_t *src_mac,
                          const uint8_t *dst_mac) const {
    libnet_ptag_t tag;

    uint8_t icmp_type = 0;
    if (pkt.get_proto_state() == PS_ICMP_ECHO_REQ) {
        icmp_type = ICMP_ECHO;
    } else if (pkt.get_proto_state() == PS_ICMP_ECHO_REP) {
        icmp_type = ICMP_ECHOREPLY;
    } else {
        Logger::error("proto_state is not related to ICMP echo");
    }

    tag = libnet_build_icmpv4_echo(icmp_type, // ICMP type
                                   0,         // ICMP code
                                   0,         // checksum (0: autofill)
                                   42,        // identification number
                                   0,         // packet sequence number
                                   NULL,      // payload
                                   0,         // payload length
                                   l,         // libnet context
                                   0);        // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build TCP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H, // length
        0,                                    // ToS
        42,                                   // identification number
        0,                                    // fragmentation offset
        64,                                   // TTL (time to live)
        IPPROTO_ICMP,                         // upper layer protocol
        0,                                    // checksum (0: autofill)
        htonl(pkt.get_src_ip().get_value()),  // source address
        htonl(pkt.get_dst_ip().get_value()),  // destination address
        NULL,                                 // payload
        0,                                    // payload length
        l,                                    // libnet context
        0);                                   // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build IP header: ") +
                      libnet_geterror(l));
    }

    tag = libnet_build_ethernet(dst_mac,      // destination ethernet address
                                src_mac,      // source ethernet address
                                ETHERTYPE_IP, // upper layer protocol
                                NULL,         // payload
                                0,            // payload length
                                l,            // libnet context
                                0);           // ptag (0: build a new one)
    if (tag < 0) {
        Logger::error(std::string("Can't build ethernet header: ") +
                      libnet_geterror(l));
    }
}

void Net::serialize(uint8_t **buffer,
                    uint32_t *buffer_size,
                    const Packet &pkt,
                    const uint8_t *src_mac,
                    const uint8_t *dst_mac) const {
    int proto_state = pkt.get_proto_state();

    if (PS_IS_TCP(proto_state)) {
        build_tcp(pkt, src_mac, dst_mac);
    } else if (PS_IS_UDP(proto_state)) {
        build_udp(pkt, src_mac, dst_mac);
    } else if (PS_IS_ICMP_ECHO(proto_state)) {
        build_icmp_echo(pkt, src_mac, dst_mac);
    } else {
        Logger::error("Unsupported packet state " +
                      std::to_string(pkt.get_proto_state()));
    }

    if (libnet_adv_cull_packet(l, buffer, buffer_size) < 0) {
        Logger::error(std::string("libnet_adv_cull_packet() failed: ") +
                      libnet_geterror(l));
    }

    libnet_clear_packet(l);
}

void Net::free(uint8_t *buffer) const {
    libnet_adv_free_packet(l, buffer);
}

void Net::deserialize(Packet &pkt, const uint8_t *buffer) const {
    // filter out irrelevant frames
    const uint8_t *dst_mac = buffer;
    const uint8_t *src_mac = buffer + 6;
    uint8_t id_mac[6] = ID_ETH_ADDR;
    if (memcmp(dst_mac, id_mac, 6) != 0 && memcmp(src_mac, id_mac, 6) != 0) {
        goto bad_packet;
    }

    pkt.clear();

    uint16_t ethertype;
    memcpy(&ethertype, buffer + 12, 2);
    ethertype = ntohs(ethertype);
    if (ethertype == ETHERTYPE_IP) { // IPv4 packets (buffer + 14)
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
        if (ip_proto == IPPROTO_TCP) { // TCP packets
            // source and destination port
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
            pkt.set_proto_state(flags | 0x80U);
            /*
             * NOTE:
             * Store the TCP flags in proto_state for now, which will be
             * converted to the real proto_state in ForwardingProcess (calling
             * Net::convert_proto_state), because the knowledge of the current
             * connection state is required to interprete the proto_state. The
             * highest bit of the variable is used to indicate unconverted TCP
             * flags.
             */
        } else if (ip_proto == IPPROTO_UDP) { // UDP packets
            // source and destination port
            uint16_t src_port, dst_port;
            memcpy(&src_port, buffer + 34, 2);
            memcpy(&dst_port, buffer + 36, 2);
            src_port = ntohs(src_port);
            dst_port = ntohs(dst_port);
            pkt.set_src_port(src_port);
            pkt.set_dst_port(dst_port);
            // UDP proto_state
            pkt.set_proto_state(PS_UDP_REQ);
            /*
             * NOTE:
             * Since UDP is connection-less, there is no way to know the actual
             * proto_state. Store PS_UDP_REQ for now, which will be converted to
             * the correct state in ForwardingProcess (calling
             * Net::convert_proto_state) based on the connection matching
             * information.
             */
        } else if (ip_proto == IPPROTO_ICMP) { // ICMP packets
            // ICMP type
            uint8_t icmp_type;
            memcpy(&icmp_type, buffer + 34, 1);
            if (icmp_type == ICMP_ECHO) {
                pkt.set_proto_state(PS_ICMP_ECHO_REQ);
            } else if (icmp_type == ICMP_ECHOREPLY) {
                pkt.set_proto_state(PS_ICMP_ECHO_REP);
            } else {
                Logger::warn("ICMP type: " + std::to_string(icmp_type));
                goto bad_packet;
            }
        } else { // unsupported L4 (or L3.5) protocols
            goto bad_packet;
        }
    } else { // unsupported L3 protocol
        goto bad_packet;
    }

    return;

bad_packet:
    pkt.clear();
}

void Net::deserialize(Packet &pkt, const PktBuffer &pb) const {
    deserialize(pkt, pb.get_buffer());
    if (!pkt.empty()) {
        pkt.set_intf(pb.get_intf());
    }
}

void Net::convert_proto_state(Packet &pkt,
                              bool is_new,
                              bool change_direction,
                              uint8_t old_proto_state) const {
    // the highest bit (0x80) is used to indicate unconverted TCP flags
    if (pkt.get_proto_state() & 0x80U) {
        uint8_t proto_state = 0;
        uint8_t flags = pkt.get_proto_state() & (~0x80U);
        if (is_new) {
            assert(flags == TH_SYN);
        }
        if (flags == TH_SYN) {
            proto_state = PS_TCP_INIT_1;
        } else if (flags == (TH_SYN | TH_ACK)) {
            proto_state = PS_TCP_INIT_2;
        } else if (flags == TH_ACK) {
            switch (old_proto_state) {
            case PS_TCP_INIT_2:
            case PS_TCP_L7_REQ:
            case PS_TCP_L7_REP:
            case PS_TCP_TERM_2:
                proto_state = old_proto_state + 1;
                assert(change_direction);
                break;
            case PS_TCP_INIT_3:
            case PS_TCP_L7_REQ_A:
            case PS_TCP_L7_REP_A:
            case PS_TCP_TERM_3:
                proto_state = old_proto_state;
                assert(!change_direction);
                break;
            default:
                Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_PUSH | TH_ACK)) {
            switch (old_proto_state) {
            case PS_TCP_INIT_3:
            case PS_TCP_L7_REQ_A:
                proto_state = old_proto_state + 1;
                assert(!change_direction);
                break;
            case PS_TCP_L7_REQ:
            case PS_TCP_L7_REP:
                proto_state = old_proto_state;
                assert(!change_direction);
                break;
            default:
                Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_FIN | TH_ACK)) {
            switch (old_proto_state) {
            case PS_TCP_L7_REP_A:
                proto_state = old_proto_state + 1;
                assert(change_direction);
                break;
            case PS_TCP_TERM_1:
                if (change_direction) {
                    proto_state = old_proto_state + 1;
                } else {
                    proto_state = old_proto_state;
                }
                break;
            case PS_TCP_TERM_2:
                proto_state = old_proto_state;
                assert(!change_direction);
                break;
            default:
                Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else {
            Logger::error("Invalid TCP flags: " + std::to_string(flags));
        }
        pkt.set_proto_state(proto_state);
    } else if (PS_IS_UDP(pkt.get_proto_state())) {
        if (is_new) {
            pkt.set_proto_state(PS_UDP_REQ);
        } else if (change_direction) {
            pkt.set_proto_state(old_proto_state + 1);
            assert(old_proto_state + 1 == PS_UDP_REP);
        } else {
            pkt.set_proto_state(old_proto_state);
        }
    } else if (PS_IS_ICMP_ECHO(pkt.get_proto_state())) {
        if (is_new) {
            assert(pkt.get_proto_state() == PS_ICMP_ECHO_REQ);
        }
    }
}

std::string Net::mac_to_str(const uint8_t *mac) const {
    std::stringstream ss;

    ss << std::setfill('0') << std::setw(2) << std::hex << mac[0];
    for (int i = 1; i < 6; ++i) {
        ss << ":" << std::setfill('0') << std::setw(2) << std::hex << mac[i];
    }

    return ss.str();
}

static void write_value_to_file(int val, const std::string &filename) {
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

void Net::set_forwarding(int val) const {
    write_value_to_file(val, "/proc/sys/net/ipv4/conf/all/forwarding");
}

void Net::set_rp_filter(int val) const {
    // The max value from conf/{all,interface}/rp_filter is used when doing
    // source validation on the {interface}.

    // set conf.all.rp_filter
    write_value_to_file(val, "/proc/sys/net/ipv4/conf/all/rp_filter");

    // set conf.{interface}.rp_filter
    struct if_nameindex *if_nidxs = if_nameindex();
    assert(if_nidxs);
    for (struct if_nameindex *intf = if_nidxs; intf->if_name != NULL; intf++) {
        write_value_to_file(val, "/proc/sys/net/ipv4/conf/" +
                                     std::string(intf->if_name) + "/rp_filter");
    }
    if_freenameindex(if_nidxs);
}

void Net::set_expire_nodest_conn(int val) const {
    write_value_to_file(val, "/proc/sys/net/ipv4/vs/expire_nodest_conn");
}
