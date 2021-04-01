#include "conn.hpp"

#include "packet.hpp"
#include "eqclassmgr.hpp"
#include "network.hpp"
#include "protocols.hpp"
#include "process/process.hpp"
#include "process/forwarding.hpp"
#include "choices.hpp"
#include "model-access.hpp"
#include "model.h"

Connection::Connection(int protocol, Node *src_node, EqClass *dst_ip_ec,
                       uint16_t src_port, uint16_t dst_port)
    : protocol(protocol), src_node(src_node), dst_ip_ec(dst_ip_ec),
      src_port(src_port), dst_port(dst_port), src_ip(0U), seq(0), ack(0)
{
}

Connection::Connection(const Packet& pkt, Node *src_node)
    : protocol(PS_TO_PROTO(pkt.get_proto_state())), src_node(src_node),
      dst_ip_ec(EqClassMgr::get().find_ec(pkt.get_dst_ip())),
      src_port(pkt.get_src_port()), dst_port(pkt.get_dst_port()),
      src_ip(pkt.get_src_ip()), seq(pkt.get_seq()), ack(pkt.get_ack())
{
}

std::string Connection::to_string() const
{
    std::string ret = "[" + proto_str(protocol) + "] "
                      + src_node->get_name() + ":" + std::to_string(src_port)
                      + " --> " + dst_ip_ec->to_string() + ":"
                      + std::to_string(dst_port);
    return ret;
}

void Connection::init(State *state, size_t conn_idx, const Network& network) const
{
    int orig_conn = get_conn(state);
    set_conn(state, conn_idx);

    set_executable(state, 2);

    uint8_t proto_state = 0;
    if (protocol == proto::tcp) {
        proto_state = PS_TCP_INIT_1;
    } else if (protocol == proto::udp) {
        proto_state = PS_UDP_REQ;
    } else if (protocol == proto::icmp_echo) {
        proto_state = PS_ICMP_ECHO_REQ;
    } else {
        Logger::error("Unknown protocol: " + std::to_string(protocol));
    }
    set_proto_state(state, proto_state);
    set_src_ip(state, src_ip.get_value());
    set_dst_ip_ec(state, dst_ip_ec);
    set_src_port(state, src_port);
    set_dst_port(state, dst_port);
    set_seq(state, seq);
    set_ack(state, ack);
    set_src_node(state, src_node);
    set_tx_node(state, src_node);
    set_rx_node(state, nullptr);

    set_fwd_mode(state, fwd_mode::FIRST_COLLECT);
    set_pkt_location(state, src_node);
    set_ingress_intf(state, nullptr);
    reset_candidates(state);

    network.update_fib(state);
    set_path_choices(state, Choices());

    // restore the original conn idx
    set_conn(state, orig_conn);
}

bool operator<(const Connection& a, const Connection& b)
{
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
