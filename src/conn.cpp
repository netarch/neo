#include "conn.hpp"

#include "choices.hpp"
#include "eqclassmgr.hpp"
#include "model-access.hpp"
#include "packet.hpp"
#include "payloadmgr.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

Connection::Connection(int protocol,
                       Node *src_node,
                       EqClass *dst_ip_ec,
                       uint16_t src_port,
                       uint16_t dst_port)
    : protocol(protocol), src_node(src_node), dst_ip_ec(dst_ip_ec),
      src_port(src_port), dst_port(dst_port), src_ip(0U), seq(0), ack(0) {}

Connection::Connection(const Packet &pkt, Node *src_node)
    : protocol(PS_TO_PROTO(pkt.get_proto_state())), src_node(src_node),
      dst_ip_ec(EqClassMgr::get().find_ec(pkt.get_dst_ip())),
      src_port(pkt.get_src_port()), dst_port(pkt.get_dst_port()),
      src_ip(pkt.get_src_ip()), seq(pkt.get_seq()), ack(pkt.get_ack()) {}

std::string Connection::to_string() const {
    std::string ret = "[" + proto_str(protocol) + "] " + src_node->get_name() +
                      ":" + std::to_string(src_port) + " --> " +
                      dst_ip_ec->to_string() + ":" + std::to_string(dst_port);
    return ret;
}

void Connection::init(size_t conn_idx) const {
    int orig_conn = model.get_conn();
    model.set_conn(conn_idx);

    model.set_executable(2);

    uint8_t proto_state = 0;
    if (protocol == proto::tcp) {
        proto_state = PS_TCP_INIT_1;
    } else if (protocol == proto::udp) {
        proto_state = PS_UDP_REQ;
    } else if (protocol == proto::icmp_echo) {
        proto_state = PS_ICMP_ECHO_REQ;
    } else {
        logger.error("Unknown protocol: " + std::to_string(protocol));
    }
    model.set_proto_state(proto_state);
    model.set_src_ip(src_ip.get_value());
    model.set_dst_ip_ec(dst_ip_ec);
    model.set_src_port(src_port);
    model.set_dst_port(dst_port);
    model.set_seq(seq);
    model.set_ack(ack);
    model.set_payload(PayloadMgr::get().get_payload_from_model());
    model.set_src_node(src_node);
    model.set_tx_node(src_node);
    model.set_rx_node(nullptr);

    model.set_fwd_mode(fwd_mode::FIRST_COLLECT);
    model.set_pkt_location(src_node);
    model.set_ingress_intf(nullptr);
    model.reset_candidates();
    model.reset_injection_results();

    model.update_fib();
    model.set_path_choices(Choices());

    // restore the original conn idx
    model.set_conn(orig_conn);
}

bool operator<(const Connection &a, const Connection &b) {
    if (a.protocol < b.protocol) {
        return true;
    } else if (a.protocol > b.protocol) {
        return false;
    }

    if (a.src_node < b.src_node) {
        return true;
    } else if (a.src_node > b.src_node) {
        return false;
    }

    if (a.dst_ip_ec < b.dst_ip_ec) {
        return true;
    } else if (a.dst_ip_ec > b.dst_ip_ec) {
        return false;
    }

    if (a.src_port < b.src_port) {
        return true;
    } else if (a.src_port > b.src_port) {
        return false;
    }

    if (a.dst_port < b.dst_port) {
        return true;
    }

    return false;
}
