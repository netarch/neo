#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include "packet.hpp"

struct nl_sock;
struct nl_msg;

class DropMon {
private:
    int _family;
    bool _enabled;
    struct nl_sock *_dm_sock;

    std::unique_ptr<std::thread> _dm_thread; // dropmon listener thread
    std::atomic<bool> _stop_dm_thread;       // loop control flag
    Packet _target_pkt;          // target packet to listen for (race)
    uint64_t _drop_ts;           // kernel drop timestamp (race)
    std::mutex _mtx;             // lock for _target_pkt and _drop_ts
    std::condition_variable _cv; // for reading _drop_ts

    DropMon();
    void listen_msgs();
    void get_stats(struct nl_sock *) const;
    struct nl_msg *new_msg(uint8_t cmd, int flags, size_t hdrlen) const;
    void del_msg(struct nl_msg *) const;
    void send_msg(struct nl_sock *, struct nl_msg *) const;
    void send_msg_async(struct nl_sock *, struct nl_msg *) const;
    Packet recv_msg(struct nl_sock *, uint64_t &) const;

public:
    // Disable the copy/move constructors and the assignment operators
    DropMon(const DropMon &) = delete;
    DropMon(DropMon &&) = delete;
    DropMon &operator=(const DropMon &) = delete;
    DropMon &operator=(DropMon &&) = delete;
    ~DropMon();

    static DropMon &get();

    /**
     * @brief Initialize the module, must be called before other functions have
     * effects.
     */
    void init();

    /**
     * @brief Reset everything as if it was just default-constructed.
     */
    void teardown();

    /**
     * @brief Start kernel drop_monitor
     */
    void start();

    /**
     * @brief Stop kernel drop_monitor
     */
    void stop() const;

    /**
     * @brief Start a dropmon thread receiving packet drop messages
     *
     * It sets up a socket to listen for dropmon messages, and starts a thread
     * to listen for those messages
     *
     * @param pkt the target packet to listen for
     */
    void start_listening_for(const Packet &pkt);

    /**
     * @brief Return a non-zero timestamp if the target packet is dropped. The
     * function blocks for a timeout if no such packet drop is observed. If the
     * timeout is 0 (default), then it will block indefinitely if no packet drop
     * is observed.
     *
     * @param timeout value for the packet drop message.
     * @return timestamp (nsec) of the packet drop in kernel
     */
    uint64_t get_drop_ts(
        std::chrono::microseconds timeout = std::chrono::microseconds::zero());

    /**
     * @brief Stop the drop listener thread, reset the dropmon variables, and
     * reset the dropmon socket
     */
    void stop_listening();
};
