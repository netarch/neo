#pragma once

#include <chrono>
#include <cstdint>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "droptrace.h"
#include "packet.hpp"

struct droptrace_bpf;

class DropTrace {
private:
    bool _enabled;
    struct droptrace_bpf *_bpf;
    struct ring_buffer *_ringbuf;
    uint32_t _target_pkt_index;
    struct drop_data _target_pkt_data;

private:
    DropTrace();
    static int libbpf_print_fn(enum libbpf_print_level level,
                               const char *format,
                               va_list args);
    static int ringbuf_handler(void *ctx, void *data, size_t size);

public:
    // Disable the copy/move constructors and the assignment operators
    DropTrace(const DropTrace &) = delete;
    DropTrace(DropTrace &&) = delete;
    DropTrace &operator=(const DropTrace &) = delete;
    DropTrace &operator=(DropTrace &&) = delete;
    ~DropTrace();

    static DropTrace &get();

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
     * @brief Open and load the BPF program.
     */
    void start();

    /**
     * @brief Remove the BPF program from the kernel.
     */
    void stop();

    /**
     * @brief Set up the target packet (in BPF array), attach the BPF program,
     * and create a ring buffer to receive events from the kernel.
     *
     * @param pkt the target packet to listen for
     */
    void start_listening_for(const Packet &pkt);

    /**
     * @brief Return a non-zero timestamp if the target packet is dropped. The
     * function blocks for a timeout if no such packet drop is observed. If the
     * timeout is negative (default), then it will block indefinitely if no
     * packet drop is observed. If the timeout is zero, the function will not
     * block.
     *
     * @param timeout timeout for the packet drop message.
     * @return timestamp (nsec) of the packet drop in kernel
     */
    uint64_t get_drop_ts(
        std::chrono::microseconds timeout = std::chrono::microseconds{-1});

    /**
     * @brief Free the ring buffer, detach the BPF program, and remove the
     * target packet from the BPF array.
     */
    void stop_listening();
};
