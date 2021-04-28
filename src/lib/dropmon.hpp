#pragma once

#include <cstdint>
#include <cstddef>

struct nl_sock;
struct nl_msg;

/*
 */
class DropMon
{
private:
    int family;
    bool enabled;
    struct nl_sock *dm_sock;

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
    void    init();         // set family and enabled
    void    start() const;  // start kernel drop_monitor
    void    stop() const;   // stop kernel drop_monitor

    void    connect();
    void    disconnect();
    void    recv() const;
};
