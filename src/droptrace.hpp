#pragma once

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "packet.hpp"

struct droptrace_bpf;

class DropTrace {
private:
    struct droptrace_bpf *_bpf;
    struct ring_buffer *_ringbuf;

private:
    DropTrace();
    static int ringbuf_handler(void *ctx, void *data, size_t size);

public:
    // Disable the copy/move constructors and the assignment operators
    DropTrace(const DropTrace &) = delete;
    DropTrace(DropTrace &&) = delete;
    DropTrace &operator=(const DropTrace &) = delete;
    DropTrace &operator=(DropTrace &&) = delete;
    ~DropTrace();

    static DropTrace &get();

    void init();
    // TODO
};
