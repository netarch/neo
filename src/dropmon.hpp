#pragma once

#include <cstdint>
#include <cstddef>
#include <thread>
#include <atomic>
//#include <queue>
//#include <mutex>
//#include <condition_variable>

#include "packet.hpp"
struct nl_sock;
struct nl_msg;
class Policy;
class DropMonPipes;

/*
 */
class DropMon
{
private:
    int family;
    bool enabled;
    struct nl_sock *dm_sock;

    void recvmsg();
    static int seq_check(struct nl_msg *, void *);
    static int alert_handler(struct nl_msg *, void *);
    std::thread *listener;              // drop listener thread
    std::atomic<bool> stop_listener;    // loop control flag
    //std::queue<Packet> dropped_pkts;    // dropped packets (race)
    //std::mutex mtx;                     // lock for accessing dropped_pkts
    //std::condition_variable cv;         // for reading dropped_pkts

    struct nl_msg * new_msg(uint8_t cmd, int flags, size_t hdrlen) const;
    void            del_msg(struct nl_msg *) const;
    void            send_msg(struct nl_sock *, struct nl_msg *) const;
    void            set_alert_mode(struct nl_sock *) const;
    void            set_queue_length(struct nl_sock *, int) const;
    void            set_sw_hw_drops(struct nl_msg *) const;
    DropMon();

private:
    friend class Config;

public:
    // Disable the copy constructor and the copy assignment operator
    DropMon(const DropMon&) = delete;
    DropMon& operator=(const DropMon&) = delete;
    ~DropMon();

    static DropMon& get();

    bool    is_enabled() const;
    void    init();             // set family and enabled
    void    start() const;      // start kernel drop_monitor
    void    stop() const;       // stop kernel drop_monitor
    void    spawn(Policy *, DropMonPipes&);

    void    connect();
    void    disconnect();
};
