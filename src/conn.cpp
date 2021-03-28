#include "comm.hpp"

#include "protocols.hpp"
#include "process/process.hpp"
#include "process/forwarding.hpp"
#include "model-access.hpp"
#include "model.h"

Connection::Connection()
    : protocol(0), owned_dst_only(false), initial_ec(nullptr),
      src_port(0), dst_port(0)
{
}

void init(State *state, size_t conn_idx, const Network& network) const
{
    int orig_conn = get_conn(state);
    set_conn(state, conn_idx);

    set_is_executable(state, true);
    set_process_id(state, pid::forwarding);

    int proto_state = 0;
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
    set_src_ip(state, 0);
    set_dst_ip_ec(state, dst_ip_ec);
    set_src_port(state, src_port);
    set_dst_port(state, dst_port);
    set_seq(state, 0);
    set_ack(state, 0);
    set_src_node(state, src_node);
    set_tx_node(state, src_node);
    set_rx_node(state, nullptr);

    set_fwd_mode(state, fwd_mode::FIRST_COLLECT);
    set_pkt_location(state, src_node);
    set_ingress_intf(state, nullptr);
    set_candidates(state, nullptr);

    network.update_fib(state);
    set_path_choices(state, Choices());

    // restore the original conn idx
    set_conn(state, orig_conn);
}
