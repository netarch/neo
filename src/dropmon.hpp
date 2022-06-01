#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>

#include "packet.hpp"
struct nl_sock;
struct nl_msg;

class DropMon {
private:
    int family;
    bool enabled;
    struct nl_sock *dm_sock;
    std::thread *listener;           // drop listener thread
    std::atomic<bool> stop_listener; // loop control flag

    Packet sent_pkt;                    // packet just sent (race)
    std::atomic<bool> sent_pkt_changed; // true if sent_pkt is modified
    std::mutex pkt_mtx;                 // lock for accessing sent_pkt

    uint64_t drop_ts;                // kernel drop timestamp (race)
    std::mutex drop_mtx;             // lock for accessing drop_ts
    std::condition_variable drop_cv; // for reading drop_ts

    void listen_msgs();
    struct nl_msg *new_msg(uint8_t cmd, int flags, size_t hdrlen) const;
    void del_msg(struct nl_msg *) const;
    void send_msg(struct nl_sock *, struct nl_msg *) const;
    void send_msg_async(struct nl_sock *, struct nl_msg *) const;
    Packet recv_msg(struct nl_sock *, uint64_t &) const;
    void set_alert_mode(struct nl_sock *) const;
    void set_queue_length(struct nl_sock *, int) const;
    void get_stats(struct nl_sock *) const;
    DropMon();

private:
    friend class Config;

public:
    // Disable the copy constructor and the copy assignment operator
    DropMon(const DropMon &) = delete;
    DropMon &operator=(const DropMon &) = delete;
    ~DropMon();

    static DropMon &get();

    /* called by the plankton main process */
    void init(bool);    // set family and enabled
    void start() const; // start kernel drop_monitor
    void stop() const;  // stop kernel drop_monitor
    /* called by each per-connection-EC process */
    void connect();    // spawn a dropmon thread and connect the socket
    void disconnect(); // join thread and disconnect
    /* called by each emulation in the main thread */
    void start_listening_for(const Packet &);
    void stop_listening();
    /* called by each emulation in the drop_listener thread */
    uint64_t is_dropped(); // return true if sent_pkt is dropped (blocking)
};
