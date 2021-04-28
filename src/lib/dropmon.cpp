#include "lib/dropmon.hpp"

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/net_dropmon.h>

#include "lib/logger.hpp"

struct nl_msg *DropMon::new_msg(uint8_t cmd, int flags, size_t hdrlen) const
{
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        Logger::error("nlmsg_alloc failed");
    }
    genlmsg_put(msg, 0, NL_AUTO_SEQ, family, hdrlen, flags, cmd, 2);
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

void DropMon::set_alert_mode(struct nl_sock *sock) const
{
    struct nl_msg *msg = new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST|NLM_F_ACK, 0);
    int err = nla_put_u8(msg, NET_DM_ATTR_ALERT_MODE, NET_DM_ALERT_MODE_PACKET);
    if (err < 0) {
        Logger::error("nla_put_u8: " + std::string(nl_geterror(err)));
    }
    send_msg(sock, msg);
    del_msg(msg);
    Logger::info("Set dropmon alert mode: packet");
}

void DropMon::set_queue_length(struct nl_sock *sock, int qlen) const
{
    struct nl_msg *msg = new_msg(NET_DM_CMD_CONFIG, NLM_F_REQUEST|NLM_F_ACK, 0);
    int err = nla_put_u32(msg, NET_DM_ATTR_QUEUE_LEN, qlen);
    if (err < 0) {
        Logger::error("nla_put_u32: " + std::string(nl_geterror(err)));
    }
    send_msg(sock, msg);
    del_msg(msg);
    Logger::info("Set dropmon queue length: " + std::to_string(qlen));
}

void DropMon::set_sw_hw_drops(struct nl_msg *msg) const
{
    int err;
    err = nla_put_flag(msg, NET_DM_ATTR_SW_DROPS);
    if (err < 0) {
        Logger::error("nla_put_flag: " + std::string(nl_geterror(err)));
    }
    err = nla_put_flag(msg, NET_DM_ATTR_HW_DROPS);
    if (err < 0) {
        Logger::error("nla_put_flag: " + std::string(nl_geterror(err)));
    }
}

DropMon::DropMon(): family(0), enabled(false), dm_sock(nullptr)
{
}

DropMon::~DropMon()
{
    if (enabled && dm_sock) {
        disconnect();
    }
}

DropMon& DropMon::get()
{
    static DropMon instance;
    return instance;
}

bool DropMon::is_enabled() const
{
    return enabled;
}

void DropMon::init()
{
    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);

    // find the generic netlink family id of NET_DM
    family = genl_ctrl_resolve(sock, "NET_DM");
    if (family < 0) {
        Logger::error("genl_ctrl_resolve NET_DM: " + std::string(nl_geterror(family)));
    }

    nl_socket_free(sock);
    enabled = true;
}

void DropMon::start() const
{
    if (!enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);

    //set_alert_mode(sock);
    //set_queue_length(sock, 100);

    struct nl_msg *msg = new_msg(NET_DM_CMD_START, NLM_F_REQUEST|NLM_F_ACK, 0);
    //set_sw_hw_drops(msg);
    send_msg(sock, msg);
    del_msg(msg);

    nl_socket_free(sock);
    Logger::info("Drop monitor started");
}

void DropMon::stop() const
{
    if (!enabled) {
        return;
    }

    struct nl_sock *sock = nl_socket_alloc();
    genl_connect(sock);

    struct nl_msg *msg = new_msg(NET_DM_CMD_STOP, NLM_F_REQUEST|NLM_F_ACK, 0);
    //set_sw_hw_drops(msg);
    send_msg(sock, msg);
    del_msg(msg);

    nl_socket_free(sock);
    Logger::info("Drop monitor stopped");
}

void DropMon::connect()
{
    if (!enabled) {
        return;
    }

    if (dm_sock) {
        Logger::warn("drop_monitor: already connected");
        return;
    }

    dm_sock = nl_socket_alloc();
    nl_join_groups(dm_sock, NET_DM_GRP_ALERT);
    // TODO: callback
    genl_connect(dm_sock);
}

void DropMon::disconnect()
{
    if (!enabled) {
        return;
    }

    if (!dm_sock) {
        return;
    }

    nl_socket_free(dm_sock);
    dm_sock = nullptr;
}
