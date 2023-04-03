#pragma once

#include <chrono>
#include <cstdint>
#include <thread>

#include "driver/driver.hpp"
#include "packet.hpp"

class DropDetection {
protected:
    bool _enabled;

public:
    DropDetection() : _enabled(false) {}
    virtual ~DropDetection() = default;

    decltype(_enabled) enabled() const { return _enabled; }

    /**
     * @brief Initialize the module, must be called before other functions have
     * effects. If the module is uninitialized, all public functions should have
     * no effects.
     */
    virtual void init() { _enabled = true; }

    /**
     * @brief Reset everything as if it was just default-constructed.
     */
    virtual void teardown() { _enabled = false; }

    /**
     * @brief Start the underlying mechanism. Implementation dependent.
     */
    virtual void start() = 0;

    /**
     * @brief Stop the underlying mechanism. Implementation dependent.
     */
    virtual void stop() = 0;

    /**
     * @brief Start listening for the target packet. The function should be
     * called each time before sending a packet.
     *
     * @param pkt the target packet to listen for
     * @param driver if provided, the target packet will contain the
     * ingress_ifindex and netns_ino for filtering events
     */
    virtual void start_listening_for(const Packet &pkt, Driver *driver) = 0;

    /**
     * @brief Return a non-zero timestamp if the target packet is dropped. The
     * function blocks for a timeout until a such packet drop is observed. If
     * the timeout is negative (default), then it will block indefinitely if no
     * packet drop is observed. If the timeout is zero, the function will not
     * block.
     *
     * @param timeout timeout for the packet drop message.
     * @return timestamp (nsec) of the packet drop in kernel
     */
    virtual uint64_t get_drop_ts(
        std::chrono::microseconds timeout = std::chrono::microseconds{-1}) = 0;

    /**
     * @brief Unblock the thread calling get_drop_ts() immediately.
     *
     * @param t reference to the thread calling get_drop_ts().
     */
    virtual void unblock(std::thread &t) = 0;

    /**
     * @brief Stop listening for the target packet. The function should be
     * called each time after receiving a packet or a drop notification.
     */
    virtual void stop_listening() = 0;
};

extern DropDetection *drop;
