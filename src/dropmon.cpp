#include "dropmon.hpp"

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/net_dropmon.h>
#include <linux/version.h>
#include <csignal>
#include <cassert>

#include "lib/net.hpp"
#include "lib/logger.hpp"


DropMon::DropMon()
    : family(0), enabled(false), dm_sock(nullptr), listener(nullptr),
      stop_listener(false), sent_pkt_changed(false), drop_ts(0)
{
}

DropMon::~DropMon()
{
    delete listener;
    nl_socket_free(dm_sock);
}

DropMon& DropMon::get()
{
    static DropMon instance;
    return instance;
}

void DropMon::init(bool enable)
{
    if (enable) {
        struct nl_sock *sock = nl_socket_alloc();
        genl_connect(sock);
        family = genl_ctrl_resolve(sock, "NET_DM");
        if (family < 0) {
            Logger::error("genl_ctrl_resolve NET_DM: " + std::string(nl_geterror(family)));
        }
        nl_socket_free(sock);
    }

    enabled = enable;
}

void DropMon::start() const
{
    if (!enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);
    set_alert_mode(sock);
    //set_queue_length(sock, 1000);
    struct nl_msg *msg = new_msg(NET_DM_CMD_START, NLM_F_REQUEST | NLM_F_ACK, 0);
    send_msg(sock, msg);
    del_msg(msg);
    nl_socket_free(sock);
}

void DropMon::stop() const
{
    if (!enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);
    struct nl_msg *msg = new_msg(NET_DM_CMD_STOP, NLM_F_REQUEST | NLM_F_ACK, 0);
    send_msg(sock, msg);
    del_msg(msg);
    nl_socket_free(sock);
}

void DropMon::connect()
{
    if (!enabled) {
        return;
    }

    if (dm_sock) {
        Logger::error("dropmon socket already connected");
    }

    dm_sock = nl_socket_alloc();
    //nl_socket_disable_seq_check(dm_sock);
    nl_join_groups(dm_sock, NET_DM_GRP_ALERT);
    genl_connect(dm_sock);

    // spawn the drop listener thread
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &old_mask);
    listener = new std::thread(&DropMon::listen_msgs, this);
    pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
}

void DropMon::disconnect()
{
    if (!enabled) {
        return;
    }

    if (listener) {
        stop_listener = true;
        get_stats(dm_sock); // in order to break out nl_recv
        if (listener->joinable()) {
            listener->join();
        }
    }
    delete listener;
    nl_socket_free(dm_sock);
    dm_sock = nullptr;
    listener = nullptr;
    stop_listener = false;
    sent_pkt_changed = false;
}

void DropMon::start_listening_for(const Packet& sent_pkt)
{
    if (!enabled) {
        return;
    }

    // switch to parsing drop messages and checking for the sent packet
    std::unique_lock<std::mutex> lck(pkt_mtx);
    this->sent_pkt = sent_pkt;
    sent_pkt_changed = true;
    lck.unlock();
}

void DropMon::stop_listening()
{
    if (!enabled) {
        return;
    }

    // switch to ignoring drop messages
    std::unique_lock<std::mutex> lck(pkt_mtx);
    this->sent_pkt.clear();
    sent_pkt_changed = true;
    lck.unlock();

    // reset the drop flag
    std::unique_lock<std::mutex> drop_lck(drop_mtx);
    drop_ts = 0;
    drop_lck.unlock();
}

uint64_t DropMon::is_dropped()
{
    if (!enabled) {
        return false;
    }

    std::unique_lock<std::mutex> lck(drop_mtx);
    drop_cv.wait(lck);
    return drop_ts;
}


/**
 * private helper functions
 */

void DropMon::listen_msgs()
{
    if (!dm_sock) {
        Logger::error("dropmon socket not connected");
    }

    Packet sent_pkt, dropped_pkt;
    uint64_t ts;

    while (!stop_listener) {
        dropped_pkt = recv_msg(dm_sock, ts);

        if (sent_pkt_changed) {
            std::unique_lock<std::mutex> lck(pkt_mtx);
            sent_pkt = this->sent_pkt;
            sent_pkt_changed = false;
            lck.unlock();
        }

        if (!sent_pkt.empty()) {
            if (dropped_pkt.same_header(sent_pkt)) {
                std::unique_lock<std::mutex> lck(drop_mtx);
                drop_ts = ts;
                drop_cv.notify_all();
            }
        }
    }
}

struct nl_msg *DropMon::new_msg(uint8_t cmd, int flags, size_t hdrlen) const {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg)
    {
        Logger::error("nlmsg_alloc failed");
    }
    genlmsg_put(msg, 0, NL_AUTO_SEQ, family, hdrlen, flags, cmd, 1);
    return msg;
}

void DropMon::del_msg(struct nl_msg *msg) const
{
    nlmsg_free(msg);
}

void DropMon::send_msg(struct nl_sock *sock, struct nl_msg *msg) const
{
    int err;
    err = nl_send_auto(sock, msg);
    if (err < 0) {
        Logger::error("nl_send_auto: " + std::string(nl_geterror(err)));
    }
    err = nl_wait_for_ack(sock);
    if (err < 0) {
        Logger::error("nl_wait_for_ack: " + std::string(nl_geterror(err)));
    }
}

void DropMon::send_msg_async(struct nl_sock *sock, struct nl_msg *msg) const
{
    int err;
    err = nl_send_auto(sock, msg);
    if (err < 0) {
        Logger::error("nl_send_auto: " + std::string(nl_geterror(err)));
    }
}

static struct nla_policy net_dm_policy[NET_DM_ATTR_MAX + 1] = {
    [NET_DM_ATTR_UNSPEC]                = { 0, 0, 0 },
    [NET_DM_ATTR_ALERT_MODE]            = { .type = NLA_U8,     0, 0 },
    [NET_DM_ATTR_PC]                    = { .type = NLA_U64,    0, 0 },
    [NET_DM_ATTR_SYMBOL]                = { .type = NLA_STRING, 0, 0 },
    [NET_DM_ATTR_IN_PORT]               = { .type = NLA_NESTED, 0, 0 },
    [NET_DM_ATTR_TIMESTAMP]             = { .type = NLA_U64,    0, 0 },
    [NET_DM_ATTR_PROTO]                 = { .type = NLA_U16,    0, 0 },
    [NET_DM_ATTR_PAYLOAD]               = { .type = NLA_UNSPEC, 0, 0 },
    [NET_DM_ATTR_PAD]                   = { 0, 0, 0 },
    [NET_DM_ATTR_TRUNC_LEN]             = { .type = NLA_U32,    0, 0 },
    [NET_DM_ATTR_ORIG_LEN]              = { .type = NLA_U32,    0, 0 },
    [NET_DM_ATTR_QUEUE_LEN]             = { .type = NLA_U32,    0, 0 },
    [NET_DM_ATTR_STATS]                 = { .type = NLA_NESTED, 0, 0 },
    [NET_DM_ATTR_HW_STATS]              = { .type = NLA_NESTED, 0, 0 },
    [NET_DM_ATTR_ORIGIN]                = { .type = NLA_U16,    0, 0 },
    [NET_DM_ATTR_HW_TRAP_GROUP_NAME]    = { .type = NLA_STRING, 0, 0 },
    [NET_DM_ATTR_HW_TRAP_NAME]          = { .type = NLA_STRING, 0, 0 },
    [NET_DM_ATTR_HW_ENTRIES]            = { .type = NLA_NESTED, 0, 0 },
    [NET_DM_ATTR_HW_ENTRY]              = { .type = NLA_NESTED, 0, 0 },
    [NET_DM_ATTR_HW_TRAP_COUNT]         = { .type = NLA_U32,    0, 0 },
    [NET_DM_ATTR_SW_DROPS]              = { 0, 0, 0 },
    [NET_DM_ATTR_HW_DROPS]              = { 0, 0, 0 },
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
    [NET_DM_ATTR_FLOW_ACTION_COOKIE]    = { 0, 0, 0 },
#endif
};

Packet DropMon::recv_msg(struct nl_sock *sock, uint64_t& ts) const
{
    int nbytes, err;
    struct sockaddr_nl addr;    // message source address
    struct nlmsghdr *nlh;       // message buffer
    struct nlattr *attrs[NET_DM_ATTR_MAX + 1];
    Packet pkt;
    void *payload;

    // receive netlink messages (Note: nl_recv will retry on EINTR)
    nbytes = nl_recv(sock, &addr, (unsigned char **)&nlh, nullptr);
    if (nbytes < 0) {
        Logger::error("nl_recv: " + std::string(nl_geterror(nbytes)));
    } else if (nbytes == 0) {
        Logger::warn("nl_recv: socket disconnected");
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
        Logger::warn("genlmsg_parse: " + std::string(nl_geterror(err)));
        goto out_free;
    }
    // filter out messages without packet payload
    if (!attrs[NET_DM_ATTR_PAYLOAD]) {
        goto out_free;
    }
    // deserialize packet
    payload = nla_data(attrs[NET_DM_ATTR_PAYLOAD]);
    Net::get().deserialize(pkt, (const uint8_t *)payload);

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

void DropMon::set_alert_mode(struct nl_sock *sock) const
{
    struct nl_msg *msg = new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST | NLM_F_ACK, 0);
    int err = nla_put_u8(msg, NET_DM_ATTR_ALERT_MODE, NET_DM_ALERT_MODE_PACKET);
    if (err < 0) {
        Logger::error("nla_put_u8: " + std::string(nl_geterror(err)));
    }
    send_msg(sock, msg);
    del_msg(msg);
}

void DropMon::set_queue_length(struct nl_sock *sock, int qlen) const
{
    struct nl_msg *msg = new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST | NLM_F_ACK, 0);
    int err = nla_put_u32(msg, NET_DM_ATTR_QUEUE_LEN, qlen);
    if (err < 0) {
        Logger::error("nla_put_u32: " + std::string(nl_geterror(err)));
    }
    send_msg(sock, msg);
    del_msg(msg);
}

void DropMon::get_stats(struct nl_sock *sock) const
{
    struct nl_msg *msg = new_msg(NET_DM_CMD_STATS_GET, NLM_F_REQUEST, 0);
    send_msg_async(sock, msg);
    del_msg(msg);
}
