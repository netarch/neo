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

#include "emulation.hpp"
#include "eqclass.hpp"
#include "lib/logger.hpp"
#include "model-access.hpp"
#include "packet.hpp"
#include "payload.hpp"
#include "pktbuffer.hpp"
#include "protocols.hpp"

using namespace std;

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

void Net::deserialize(Packet &pkt, const uint8_t *buffer, size_t buflen) const {
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
            pkt.set_seq(seq);
            pkt.set_ack(ack);
            // Data offset (header length)
            uint8_t offset;
            memcpy(&offset, buffer + 46, 1);
            offset = offset >> 4;
            // TCP flags
            uint16_t flags;
            memcpy(&flags, buffer + 46, 2);
            flags = ntohs(flags);
            flags &= 0x0fff;
            pkt.set_proto_state(flags | 0x800U);
            /**
             * NOTE:
             * Store the TCP flags in proto_state for now, which will be
             * converted to the real proto_state later (calling
             * Net::convert_proto_state), because the knowledge of the current
             * connection state is required to interprete the proto_state and
             * also we don't need to convert it for rewinding packets. The
             * highest bit of the variable is used to indicate unconverted TCP
             * flags.
             */
            // Payload
            int dataoff = 34 + int(offset) * 4;
            size_t datalen = buflen - dataoff;
            if (datalen == 0) {
                pkt.set_payload(nullptr);
            } else {
                uint8_t *data = new uint8_t[datalen];
                memcpy(data, buffer + dataoff, datalen);
                pkt.set_payload(PayloadMgr::get().get_payload(data, datalen));
            }
        } else if (ip_proto == IPPROTO_UDP) { // UDP packets
            // source and destination port
            uint16_t src_port, dst_port;
            memcpy(&src_port, buffer + 34, 2);
            memcpy(&dst_port, buffer + 36, 2);
            src_port = ntohs(src_port);
            dst_port = ntohs(dst_port);
            pkt.set_src_port(src_port);
            pkt.set_dst_port(dst_port);
            // length
            uint16_t length;
            memcpy(&length, buffer + 38, 2);
            length -= 8;
            length = min(length, uint16_t(buflen - 42));
            // UDP proto_state
            pkt.set_proto_state(PS_UDP_REQ);
            /**
             * NOTE:
             * Since UDP is connection-less, there is no way to know the actual
             * proto_state. Store PS_UDP_REQ for now, which will be converted to
             * the correct state later (calling Net::convert_proto_state) based
             * on the connection matching information.
             */
            // Payload
            if (length == 0) {
                pkt.set_payload(nullptr);
            } else {
                uint8_t *data = new uint8_t[length];
                memcpy(data, buffer + 42, length);
                pkt.set_payload(PayloadMgr::get().get_payload(data, length));
            }
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
            // TODO
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
    deserialize(pkt, pb.get_buffer(), pb.get_len());
    if (!pkt.empty()) {
        pkt.set_intf(pb.get_intf());
    }
}

bool Net::is_tcp_ack_or_psh_ack(const Packet &pkt) const {
    assert(pkt.get_proto_state() & 0x800U);
    return (pkt.get_proto_state() == (0x800U | TH_ACK) ||
            pkt.get_proto_state() == (0x800U | TH_PUSH | TH_ACK));
}

void Net::convert_proto_state(Packet &pkt, uint16_t old_proto_state) const {
    // the highest bit (0x800U) is used to indicate unconverted TCP flags
    if (pkt.get_proto_state() & 0x800U) {
        uint16_t proto_state = 0;
        uint16_t flags = pkt.get_proto_state() & (~0x800U);
        size_t payloadlen =
            pkt.get_payload() ? pkt.get_payload()->get_size() : 0;

        if (pkt.is_new()) {
            assert(flags == TH_SYN);
        }

        if (flags == TH_SYN) {
            proto_state = PS_TCP_INIT_1;
        } else if (flags == (TH_SYN | TH_ACK)) {
            proto_state = PS_TCP_INIT_2;
        } else if (flags == TH_ACK || flags == (TH_PUSH | TH_ACK)) {
            switch (old_proto_state) {
            case PS_TCP_INIT_2:
            case PS_TCP_TERM_2:
                proto_state = old_proto_state + 1;
                assert(pkt.opposite_dir());
                break;
            case PS_TCP_INIT_3:
            case PS_TCP_L7_REQ_A:
                if (payloadlen == 0) {
                    proto_state = old_proto_state;
                } else {
                    proto_state = old_proto_state + 1;
                }
                assert(!pkt.opposite_dir());
                break;
            case PS_TCP_L7_REQ:
            case PS_TCP_L7_REP:
                if (payloadlen == 0) {
                    proto_state = old_proto_state + 1;
                    assert(pkt.opposite_dir());
                } else {
                    proto_state = old_proto_state;
                    assert(!pkt.opposite_dir());
                }
                break;
            case PS_TCP_L7_REP_A:
            case PS_TCP_TERM_3:
                proto_state = old_proto_state;
                assert(payloadlen == 0);
                assert(!pkt.opposite_dir());
                break;
            default:
                Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else if (flags == (TH_FIN | TH_ACK)) {
            switch (old_proto_state) {
            case PS_TCP_INIT_3:
            case PS_TCP_L7_REQ_A:
            case PS_TCP_L7_REP_A:
                proto_state = PS_TCP_TERM_1;
                break;
            case PS_TCP_TERM_1:
                if (pkt.opposite_dir()) {
                    proto_state = PS_TCP_TERM_2;
                } else {
                    proto_state = old_proto_state;
                }
                break;
            case PS_TCP_TERM_2:
                proto_state = old_proto_state;
                assert(!pkt.opposite_dir());
                break;
            default:
                Logger::error("Invalid TCP flags: " + std::to_string(flags));
            }
        } else {
            Logger::error("Invalid TCP flags: " + std::to_string(flags));
        }
        pkt.set_proto_state(proto_state);
    } else if (PS_IS_UDP(pkt.get_proto_state())) {
        if (pkt.is_new()) {
            pkt.set_proto_state(PS_UDP_REQ);
        } else if (pkt.opposite_dir()) {
            pkt.set_proto_state(old_proto_state + 1);
            assert(old_proto_state + 1 == PS_UDP_REP);
        } else {
            pkt.set_proto_state(old_proto_state);
        }
    } else if (PS_IS_ICMP_ECHO(pkt.get_proto_state())) {
        if (pkt.is_new()) {
            assert(pkt.get_proto_state() == PS_ICMP_ECHO_REQ);
        }
    }
}

void Net::reassemble_segments(std::list<Packet> &pkts) const {
    std::unordered_map<Interface *, std::list<Packet>> intf_pkts_map;
    auto p = pkts.begin();

    while (p != pkts.end()) {
        auto &intf_pkts = intf_pkts_map[p->get_intf()];

        if (!intf_pkts.empty()) {
            auto &lp = intf_pkts.back(); // located packet

            if (lp.get_src_ip() == p->get_src_ip() &&
                lp.get_dst_ip() == p->get_dst_ip() &&
                lp.get_src_port() == p->get_src_port() &&
                lp.get_dst_port() == p->get_dst_port() &&
                Net::get().is_tcp_ack_or_psh_ack(lp) &&
                Net::get().is_tcp_ack_or_psh_ack(*p) &&
                lp.get_seq() + lp.get_payload_size() == p->get_seq() &&
                lp.get_ack() == p->get_ack()) {

                lp.set_payload(PayloadMgr::get().get_payload(lp, *p));
                pkts.erase(p++);
                continue;
            }
        }

        intf_pkts.emplace_back(std::move(*p));
        pkts.erase(p++);
    }

    assert(pkts.empty());

    for (auto &[intf, intf_pkts] : intf_pkts_map) {
        pkts.splice(pkts.end(), intf_pkts);
    }
}

void Net::identify_conn(Packet &pkt) const {
    int conn;
    int orig_conn = model.get_conn();

    for (conn = 0; conn < model.get_num_conns(); ++conn) {
        model.set_conn(conn);
        uint32_t src_ip = model.get_src_ip();
        EqClass *dst_ip_ec = model.get_dst_ip_ec();
        uint16_t src_port = model.get_src_port();
        uint16_t dst_port = model.get_dst_port();
        int conn_protocol = PS_TO_PROTO(model.get_proto_state());
        model.set_conn(orig_conn);

        /*
         * Note that at this moment we have not converted the received
         * packet's proto_state, so only protocol type is compared.
         */
        int pkt_protocol;
        if (pkt.get_proto_state() & 0x800U) {
            pkt_protocol = proto::tcp;
        } else {
            pkt_protocol = PS_TO_PROTO(pkt.get_proto_state());
        }

        if (pkt.get_src_ip() == src_ip &&
            dst_ip_ec->contains(pkt.get_dst_ip()) &&
            pkt.get_src_port() == src_port && pkt.get_dst_port() == dst_port &&
            pkt_protocol == conn_protocol) {
            // same connection, same direction
            pkt.set_is_new(false);
            pkt.set_opposite_dir(false);
            break;
        } else if (pkt.get_dst_ip() == src_ip &&
                   dst_ip_ec->contains(pkt.get_src_ip()) &&
                   pkt.get_dst_port() == src_port &&
                   pkt.get_src_port() == dst_port &&
                   pkt_protocol == conn_protocol) {
            // same connection, opposite direction
            pkt.set_is_new(false);
            pkt.set_opposite_dir(true);
            break;
        }
    }

    // new connection (NAT'd packets are also treated as new connections)
    if (conn >= model.get_num_conns()) {
        if (conn >= MAX_CONNS) {
            Logger::error("Exceeding the maximum number of connections");
        }

        pkt.set_is_new(true);
        pkt.set_opposite_dir(false);
    }

    pkt.set_conn(conn);
}

void Net::process_proto_state(Packet &pkt) const {
    uint16_t old_proto_state = model.get_proto_state_for_conn(pkt.conn());
    Net::get().convert_proto_state(pkt, old_proto_state);

    if (pkt.is_new()) {
        assert(PS_IS_FIRST(pkt.get_proto_state()));
        pkt.set_next_phase(false);
    } else {
        if (pkt.get_proto_state() == old_proto_state) {
            pkt.set_next_phase(false);
        } else if (pkt.get_proto_state() > old_proto_state) {
            pkt.set_next_phase(true);
        } else {
            Logger::warn("Invalid protocol state");
            pkt.set_proto_state(PS_INVALID);
        }
    }
}

void Net::check_seq_ack(Packet &pkt) const {
    if (!PS_IS_TCP(pkt.get_proto_state())) {
        return;
    }

    // verify seq/ack numbers
    int orig_conn = model.get_conn();
    model.set_conn(pkt.conn());

    if (pkt.is_new()) { // new connection (SYN)
        assert(pkt.get_seq() == 0);
        assert(pkt.get_ack() == 0);
    } else if (pkt.next_phase()) { // old connection; next phase
        uint8_t old_proto_state = model.get_proto_state();
        uint32_t old_seq = model.get_seq();
        uint32_t old_ack = model.get_ack();
        Payload *pl = model.get_payload();
        uint32_t old_payload_size = pl ? pl->get_size() : 0;
        if (old_payload_size > 0) {
            old_seq += old_payload_size;
        } else if (PS_HAS_SYN(old_proto_state) || PS_HAS_FIN(old_proto_state)) {
            old_seq += 1;
        }

        if (pkt.opposite_dir()) {
            assert(pkt.get_seq() == old_ack);
            assert(pkt.get_ack() == old_seq);
        } else {
            assert(pkt.get_seq() == old_seq);
            assert(pkt.get_ack() == old_ack);
        }
    } else { // old connection; same phase
        // seq/ack should remain the same
        assert(pkt.get_seq() == model.get_seq());
        assert(pkt.get_ack() == model.get_ack());
        assert(pkt.get_payload() == model.get_payload());
    }

    model.set_conn(orig_conn);
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
