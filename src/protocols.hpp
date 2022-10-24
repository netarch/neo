#pragma once

#include <string>

enum proto {
    tcp,
    udp,
    icmp_echo
};

static inline std::string proto_str(int n) {
    return n == proto::tcp         ? "TCP"
           : n == proto::udp       ? "UDP"
           : n == proto::icmp_echo ? "ICMP"
                                   : "(unknown)";
}

/*
 * Protocol state
 */
// Note that for TCP four-way termination handshake, we assume that the second
// (ACK) and third (FIN) steps are always set within the same packet.
#define PS_TCP_INIT_1 0     // TCP 3-way handshake SYN
#define PS_TCP_INIT_2 1     // TCP 3-way handshake SYN/ACK
#define PS_TCP_INIT_3 2     // TCP 3-way handshake ACK
#define PS_TCP_L7_REQ 3     // TCP L7 request
#define PS_TCP_L7_REQ_A 4   // TCP L7 request ACK
#define PS_TCP_L7_REP 5     // TCP L7 reply
#define PS_TCP_L7_REP_A 6   // TCP L7 reply ACK
#define PS_TCP_TERM_1 7     // TCP termination FIN/ACK
#define PS_TCP_TERM_2 8     // TCP termination FIN/ACK
#define PS_TCP_TERM_3 9     // TCP termination ACK
#define PS_UDP_REQ 10       // UDP request
#define PS_UDP_REP 11       // UDP reply
#define PS_ICMP_ECHO_REQ 12 // ICMP echo request
#define PS_ICMP_ECHO_REP 13 // ICMP echo reply

#define PS_IS_REQUEST_DIR(x)                                                   \
    ((x) == PS_TCP_INIT_1 || (x) == PS_TCP_INIT_3 || (x) == PS_TCP_L7_REQ ||   \
     (x) == PS_TCP_L7_REP_A || (x) == PS_TCP_TERM_2 || (x) == PS_UDP_REQ ||    \
     (x) == PS_ICMP_ECHO_REQ)
#define PS_IS_REPLY_DIR(x)                                                     \
    ((x) == PS_TCP_INIT_2 || (x) == PS_TCP_L7_REQ_A || (x) == PS_TCP_L7_REP || \
     (x) == PS_TCP_TERM_1 || (x) == PS_TCP_TERM_3 || (x) == PS_UDP_REP ||      \
     (x) == PS_ICMP_ECHO_REP)
#define PS_IS_REQUEST(x)                                                       \
    ((x) == PS_TCP_L7_REQ || (x) == PS_UDP_REQ || (x) == PS_ICMP_ECHO_REQ)
#define PS_IS_REPLY(x)                                                         \
    ((x) == PS_TCP_L7_REP || (x) == PS_UDP_REP || (x) == PS_ICMP_ECHO_REP)
#define PS_IS_FIRST(x)                                                         \
    ((x) == PS_TCP_INIT_1 || (x) == PS_UDP_REQ || (x) == PS_ICMP_ECHO_REQ)
#define PS_IS_LAST(x)                                                          \
    ((x) == PS_TCP_TERM_3 || (x) == PS_UDP_REP || (x) == PS_ICMP_ECHO_REP)
#define PS_IS_TCP(x)                                                           \
    (((x) >= PS_TCP_INIT_1 && (x) <= PS_TCP_TERM_3) || ((x)&0x800U))
#define PS_IS_UDP(x) ((x) >= PS_UDP_REQ && (x) <= PS_UDP_REP)
#define PS_IS_ICMP_ECHO(x) ((x) >= PS_ICMP_ECHO_REQ && (x) <= PS_ICMP_ECHO_REP)
#define PS_HAS_SYN(x) ((x) == PS_TCP_INIT_1 || (x) == PS_TCP_INIT_2)
#define PS_HAS_FIN(x) ((x) == PS_TCP_TERM_1 || (x) == PS_TCP_TERM_2)
#define PS_SAME_PROTO(x, y)                                                    \
    ((PS_IS_TCP(x) && PS_IS_TCP(y)) || (PS_IS_UDP(x) && PS_IS_UDP(y)) ||       \
     (PS_IS_ICMP_ECHO(x) && PS_IS_ICMP_ECHO(y)))
#define PS_SAME_DIR(x, y)                                                      \
    ((PS_IS_REQUEST_DIR(x) && PS_IS_REQUEST_DIR(y)) ||                         \
     (PS_IS_REPLY_DIR(x) && PS_IS_REPLY_DIR(y)))
#define PS_IS_NEXT(x, y) ((x) == (y) + 1 && PS_SAME_PROTO(x, y))
#define PS_LAST_PROTO_STATE(x)                                                 \
    (PS_IS_TCP(x) ? PS_TCP_TERM_3                                              \
                  : (PS_IS_UDP(x) ? PS_UDP_REP : PS_ICMP_ECHO_REP))
#define PS_TO_PROTO(x)                                                         \
    (PS_IS_TCP(x) ? proto::tcp : (PS_IS_UDP(x) ? proto::udp : proto::icmp_echo))
#define PS_STR(x)                                                              \
    (PS_IS_TCP(x)                                                              \
         ? "TCP"                                                               \
         : (PS_IS_UDP(x) ? "UDP"                                               \
                         : (PS_IS_ICMP_ECHO(x) ? "ICMP" : "(unknown)")))
