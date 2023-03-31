#include "dropmon.hpp"

#include <csignal>
#include <linux/net_dropmon.h>
#include <linux/version.h>
#include <netlink/errno.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>

#include "lib/net.hpp"
#include "logger.hpp"

using namespace std;

DropMon::DropMon()
    : _family(0), _enabled(false), _dm_sock(nullptr), _stop_dm_thread(false),
      _drop_ts(0) {}

DropMon::~DropMon() {
    teardown();
}

DropMon &DropMon::get() {
    static DropMon instance;
    return instance;
}

void DropMon::init() {
    // Reset everything
    teardown();

    // Set family
    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);
    _family = genl_ctrl_resolve(sock, "NET_DM");
    if (_family < 0) {
        logger.error("genl_ctrl_resolve NET_DM: " +
                     string(nl_geterror(_family)));
    }
    nl_socket_free(sock);

    // Enable the module
    _enabled = true;
}

void DropMon::teardown() {
    this->stop_listening();
    _family = 0;
    _enabled = false;
}

void DropMon::start() {
    if (!_enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);

    // Set alert mode
    {
        struct nl_msg *msg =
            new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST | NLM_F_ACK, 0);
        int err =
            nla_put_u8(msg, NET_DM_ATTR_ALERT_MODE, NET_DM_ALERT_MODE_PACKET);
        if (err < 0) {
            logger.error("nla_put_u8: " + string(nl_geterror(err)));
        }
        send_msg(sock, msg);
        del_msg(msg);
    }

    // Set queue length
    {
        constexpr int qlen = 3000;
        struct nl_msg *msg =
            new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST | NLM_F_ACK, 0);
        int err = nla_put_u32(msg, NET_DM_ATTR_QUEUE_LEN, qlen);
        if (err < 0) {
            logger.error("nla_put_u32: " + string(nl_geterror(err)));
        }
        send_msg(sock, msg);
        del_msg(msg);
    }

    // Start drop_monitor
    {
        struct nl_msg *msg =
            new_msg(NET_DM_CMD_START, NLM_F_REQUEST | NLM_F_ACK, 0);
        send_msg(sock, msg);
        del_msg(msg);
    }

    nl_socket_free(sock);
}

void DropMon::stop() const {
    if (!_enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);
    struct nl_msg *msg = new_msg(NET_DM_CMD_STOP, NLM_F_REQUEST | NLM_F_ACK, 0);
    send_msg(sock, msg);
    del_msg(msg);
    nl_socket_free(sock);
}

void DropMon::start_listening_for(const Packet &pkt) {
    if (!_enabled) {
        return;
    }

    // Set up the dropmon socket
    if (_dm_sock) {
        logger.error("dropmon socket is already connected");
    }
    _dm_sock = nl_socket_alloc();
    nl_socket_disable_seq_check(_dm_sock);
    nl_join_groups(_dm_sock, NET_DM_GRP_ALERT);
    genl_connect(_dm_sock);

    // Reset dropmon variables
    unique_lock<mutex> lck(_mtx);
    _target_pkt = pkt;
    _drop_ts = 0;
    lck.unlock();

    // Set up the drop listener thread (block all signals)
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
    _dm_thread = make_unique<thread>(&DropMon::listen_msgs, this);
    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
}

uint64_t DropMon::get_drop_ts(chrono::microseconds timeout) {
    if (!_enabled) {
        return 0;
    }

    unique_lock<mutex> lck(_mtx);

    if (_drop_ts != 0) {
        return _drop_ts;
    }

    if (timeout == chrono::microseconds::zero()) {
        _cv.wait(lck);
    } else {
        _cv.wait_for(lck, timeout);
    }

    return _drop_ts;
}

void DropMon::stop_listening() {
    if (!_enabled) {
        return;
    }

    // Stop the drop listener thread
    if (_dm_thread) {
        _stop_dm_thread = true;
        // Because nl_recv will retry internally when interrupted by signals, we
        // need to send a message (asynchronously) to break out of the nl_recv
        // function
        get_stats(_dm_sock);
        if (_dm_thread->joinable()) {
            _dm_thread->join();
        }
        _dm_thread.reset();
        _stop_dm_thread = false;
    }

    // Reset the dropmon variables
    unique_lock<mutex> lck(_mtx);
    _target_pkt.clear();
    _drop_ts = 0;
    lck.unlock();

    // Reset the dropmon socket
    nl_socket_free(_dm_sock);
    _dm_sock = nullptr;
}

void DropMon::listen_msgs() {
    if (!_dm_sock) {
        logger.error("dropmon socket is not connected");
    }

    unique_lock<mutex> lck(_mtx);
    if (_target_pkt.empty()) {
        logger.error("Empty target packet");
    }
    lck.unlock();

    Packet dropped_pkt;
    uint64_t ts;

    while (!_stop_dm_thread) {
        dropped_pkt = recv_msg(_dm_sock, ts);

        lck.lock();
        if (dropped_pkt.same_header(_target_pkt)) {
            _drop_ts = ts;
            _cv.notify_all();
        }
        lck.unlock();
    }
}

void DropMon::get_stats(struct nl_sock *sock) const {
    struct nl_msg *msg = new_msg(NET_DM_CMD_STATS_GET, NLM_F_REQUEST, 0);
    send_msg_async(sock, msg);
    del_msg(msg);
}

struct nl_msg *DropMon::new_msg(uint8_t cmd, int flags, size_t hdrlen) const {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        logger.error("nlmsg_alloc failed");
    }
    genlmsg_put(msg, 0, NL_AUTO_SEQ, _family, hdrlen, flags, cmd, 1);
    return msg;
}

void DropMon::del_msg(struct nl_msg *msg) const {
    nlmsg_free(msg);
}

void DropMon::send_msg(struct nl_sock *sock, struct nl_msg *msg) const {
    int err;
    err = nl_send_auto(sock, msg);
    if (err < 0) {
        logger.error("nl_send_auto: " + string(nl_geterror(err)));
    }

    do {
        err = nl_wait_for_ack(sock);
        if (err < 0) {
            if (err != -NLE_BUSY) {
                logger.error("nl_wait_for_ack: " + string(nl_geterror(err)));
            }
        } else {
            break;
        }
    } while (1);
}

void DropMon::send_msg_async(struct nl_sock *sock, struct nl_msg *msg) const {
    int err;
    err = nl_send_auto(sock, msg);
    if (err < 0) {
        logger.error("nl_send_auto: " + string(nl_geterror(err)));
    }
}

static struct nla_policy net_dm_policy[NET_DM_ATTR_MAX + 1] = {
    [NET_DM_ATTR_UNSPEC] = {NLA_UNSPEC, 0, 0},
    [NET_DM_ATTR_ALERT_MODE] = {NLA_U8,     0, 0},
    [NET_DM_ATTR_PC] = {NLA_U64,    0, 0},
    [NET_DM_ATTR_SYMBOL] = {NLA_STRING, 0, 0},
    [NET_DM_ATTR_IN_PORT] = {NLA_NESTED, 0, 0},
    [NET_DM_ATTR_TIMESTAMP] = {NLA_U64,    0, 0},
    [NET_DM_ATTR_PROTO] = {NLA_U16,    0, 0},
    [NET_DM_ATTR_PAYLOAD] = {NLA_UNSPEC, 0, 0},
    [NET_DM_ATTR_PAD] = {NLA_UNSPEC, 0, 0},
    [NET_DM_ATTR_TRUNC_LEN] = {NLA_U32,    0, 0},
    [NET_DM_ATTR_ORIG_LEN] = {NLA_U32,    0, 0},
    [NET_DM_ATTR_QUEUE_LEN] = {NLA_U32,    0, 0},
    [NET_DM_ATTR_STATS] = {NLA_NESTED, 0, 0},
    [NET_DM_ATTR_HW_STATS] = {NLA_NESTED, 0, 0},
    [NET_DM_ATTR_ORIGIN] = {NLA_U16,    0, 0},
    [NET_DM_ATTR_HW_TRAP_GROUP_NAME] = {NLA_STRING, 0, 0},
    [NET_DM_ATTR_HW_TRAP_NAME] = {NLA_STRING, 0, 0},
    [NET_DM_ATTR_HW_ENTRIES] = {NLA_NESTED, 0, 0},
    [NET_DM_ATTR_HW_ENTRY] = {NLA_NESTED, 0, 0},
    [NET_DM_ATTR_HW_TRAP_COUNT] = {NLA_U32,    0, 0},
    [NET_DM_ATTR_SW_DROPS] = {NLA_FLAG,   0, 0},
    [NET_DM_ATTR_HW_DROPS] = {NLA_FLAG,   0, 0},
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    [NET_DM_ATTR_FLOW_ACTION_COOKIE] = {NLA_UNSPEC, 0, 0},
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
    [NET_DM_ATTR_REASON] = {NLA_STRING, 0, 0},
#endif
#endif
};

Packet DropMon::recv_msg(struct nl_sock *sock, uint64_t &ts) const {
    int nbytes, err, payloadlen;
    struct sockaddr_nl addr; // message source address
    struct nlmsghdr *nlh;    // message buffer
    struct nlattr *attrs[NET_DM_ATTR_MAX + 1];
    Packet pkt;
    void *payload;

    // receive netlink messages (Note: nl_recv will retry on EINTR)
    nbytes = nl_recv(sock, &addr, (unsigned char **)&nlh, nullptr);
    if (nbytes < 0) {
        logger.error("nl_recv: " + string(nl_geterror(nbytes)));
    } else if (nbytes == 0) {
        logger.warn("nl_recv: socket disconnected");
        goto out_free;
    }
    // filter out ACKs and error messages
    if (nlh->nlmsg_type == NLMSG_ERROR) {
        goto out_free;
    }
    // filter dropped packet alerts
    if (genlmsg_hdr(nlh)->cmd != NET_DM_CMD_PACKET_ALERT) {
        goto out_free;
    }

    // parse genetlink message attributes
    err = genlmsg_parse(nlh, 0, attrs, NET_DM_ATTR_MAX, net_dm_policy);
    if (err < 0) {
        logger.warn("genlmsg_parse: " + string(nl_geterror(err)));
        goto out_free;
    }
    // filter out messages without packet payload
    if (!attrs[NET_DM_ATTR_PAYLOAD]) {
        goto out_free;
    }
    // deserialize packet
    payload = nla_data(attrs[NET_DM_ATTR_PAYLOAD]);
    payloadlen = nla_len(attrs[NET_DM_ATTR_PAYLOAD]);
    Net::get().deserialize(pkt, (const uint8_t *)payload, payloadlen);

    // timestamp
    if (!pkt.empty() && attrs[NET_DM_ATTR_TIMESTAMP]) {
        ts = nla_get_u64(attrs[NET_DM_ATTR_TIMESTAMP]);
    } else {
        ts = 0;
    }

out_free:
    free(nlh);
    return pkt;
}
