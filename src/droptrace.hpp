#pragma once

#include <chrono>
#include <cstdint>

#include "driver/driver.hpp"
#include "dropdetection.hpp"
#include "droptrace.h"
#include "packet.hpp"

struct droptrace_bpf;
struct ring_buffer;

class DropTrace : public DropDetection {
private:
    struct droptrace_bpf *_bpf;   // main BPF handle (skel)
    struct ring_buffer *_ringbuf; // ring buffer to receive events
    uint32_t _target_pkt_key;     // key for the target_packet BPF hash map
    struct drop_data _target_pkt; // value for the target_packet BPF hash map
    uint64_t _drop_ts;            // kernel drop timestamp

private:
    DropTrace();
    static int ringbuf_handler(void *ctx, void *data, size_t size);

public:
    // Disable the copy/move constructors and the assignment operators
    DropTrace(const DropTrace &)            = delete;
    DropTrace(DropTrace &&)                 = delete;
    DropTrace &operator=(const DropTrace &) = delete;
    DropTrace &operator=(DropTrace &&)      = delete;
    ~DropTrace() override;

    static DropTrace &get();

    /**
     * @brief Initialize the module, must be called before other functions have
     * effects.
     */
    void init() override;

    /**
     * @brief Reset everything as if it was just default-constructed.
     */
    void teardown() override;

    /**
     * @brief Open and load the BPF program.
     */
    void start() override;

    /**
     * @brief Remove the BPF program from the kernel.
     */
    void stop() override;

    /**
     * @brief Set up the target packet (in BPF array), attach the BPF program,
     * and create a ring buffer to receive events from the kernel.
     *
     * @param pkt the target packet to listen for
     * @param driver if provided, the target packet will contain the
     * ingress_ifindex and netns_ino for filtering events
     */
    void start_listening_for(const Packet &pkt, Driver *driver) override;

    /**
     * @brief Return a non-zero timestamp if the target packet is dropped. The
     * function blocks for a timeout until a such packet drop is observed. If
     * the timeout is negative (default), then it will block indefinitely if no
     * packet drop is observed. If the timeout is zero, the function will not
     * block.
     *
     * Note that the timeout is at the milliseconds granularity because
     * ring_buffer__poll uses the old epoll_wait API (instead of epoll_pwait2
     * which accepts struct timespec for the timeout). This shouldn't be an
     * issue since we're using this function asynchronously anyway. But if later
     * we need a more precise timeout, we can have a separate thread calling
     * ring_buffer__poll and use cv.wait_for instead, similar to how DropMon is
     * doing.
     *
     * @param timeout timeout for the packet drop message.
     * @return timestamp (nsec) of the packet drop in kernel
     */
    uint64_t get_drop_ts(std::chrono::microseconds timeout =
                             std::chrono::microseconds{-1}) override;

    /**
     * @brief Unblock the thread calling get_drop_ts() immediately.
     *
     * @param t reference to the thread calling get_drop_ts().
     */
    void unblock(std::thread &t) override;

    /**
     * @brief Free the ring buffer, detach the BPF program, and remove the
     * target packet from the BPF array.
     */
    void stop_listening() override;
};
