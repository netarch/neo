#include "dropmon.hpp"

#include <unistd.h> // sleep
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/net_dropmon.h>
#include <csignal>

#include "lib/logger.hpp"


DropMon::DropMon()
    : family(0), enabled(false), dm_sock(nullptr), listener(nullptr),
      stop_listener(false), sent_pkt_changed(false), pkt_dropped(false)
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
    //set_alert_mode(sock);
    //set_queue_length(sock, 1000);
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
        Logger::error("dropmon socket already connected");
    }

    dm_sock = nl_socket_alloc();
    nl_socket_disable_seq_check(dm_sock);
    //nl_socket_modify_cb(dm_sock, NL_CB_VALID, NL_CB_CUSTOM, DropMon::alert_handler, nullptr);
    nl_join_groups(dm_sock, NET_DM_GRP_ALERT);
    genl_connect(dm_sock);

    // spawn the drop listener thread (block all signals but SIGUSR1)
    sigset_t mask, old_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
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
        pthread_kill(listener->native_handle(), SIGUSR1);
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
    pthread_kill(listener->native_handle(), SIGUSR1);
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
    pthread_kill(listener->native_handle(), SIGUSR1);

    // reset the drop flag
    std::unique_lock<std::mutex> drop_lck(drop_mtx);
    pkt_dropped = false;
    drop_lck.unlock();
}

bool DropMon::is_dropped()
{
    if (!enabled) {
        return false;
    }

    std::unique_lock<std::mutex> lck(drop_mtx);
    drop_cv.wait(lck);
    return pkt_dropped;
}


/******************************************************************************/
/* private helper functions */
/******************************************************************************/


void DropMon::listen_msgs()
{
    if (!dm_sock) {
        Logger::error("dropmon socket not connected");
    }

    Packet pkt;

    while (!stop_listener) {
        if (sent_pkt_changed) { // switch mode if sent_pkt is modified
            std::unique_lock<std::mutex> lck(pkt_mtx);
            pkt = sent_pkt;
            sent_pkt_changed = false;
            lck.unlock();
            Logger::debug("Switch listening mode: " + std::to_string(!pkt.empty()));
        }

        if (pkt.empty()) {  // no packet sent; ignore all drop messages
            // receive the drop msgs (it will block if there is no drop)
            int err = nl_recvmsgs_default(dm_sock);
            if (err < 0) {
                if (err == -NLE_INTR) {
                    Logger::warn("drop listener thread interrupted");
                } else {
                    Logger::error("nl_recvmsgs_default: " + std::string(nl_geterror(err)));
                }
            }
        } else {            // check for the sent packet drop message
            std::unique_lock<std::mutex> lck(drop_mtx);
            bool dropped = pkt_dropped;
            lck.unlock();

            if (dropped) {
                sleep(1);
            } else {
                // TODO: receive netlink messages; check with sent_pkt

                // if the sent_pkt is dropped:
                std::unique_lock<std::mutex> lck(drop_mtx);
                pkt_dropped = true;
                drop_cv.notify_all();
            }
        }
    }
}

int DropMon::alert_handler(struct nl_msg *msg __attribute__((unused)),
                           void *arg __attribute__((unused)))
{
    //Logger::debug("alert_handler"); // TODO
    return NL_OK;
}



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
