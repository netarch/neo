#include "droptrace.hpp"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "droptrace.h"
#include "droptrace.skel.h"
#include "logger.hpp"

using namespace std;

DropTrace::DropTrace()
    : _enabled(false), _bpf(nullptr), _ringbuf(nullptr), _target_pkt_index(0) {
    memset(&_target_pkt_data, 0, sizeof(_target_pkt_data));
    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    libbpf_set_print(DropTrace::libbpf_print_fn);
}

DropTrace::~DropTrace() {
    teardown();
}

int DropTrace::libbpf_print_fn(enum libbpf_print_level level,
                               const char *format,
                               va_list args) {
    if (level >= LIBBPF_DEBUG) {
        return 0;
    }

    constexpr int buffer_sz = 2048;
    static char buffer[buffer_sz];
    int res = vsnprintf(buffer, buffer_sz, format, args);

    if (res < 0) {
        logger.error("vsnprintf() failed: " + to_string(res));
    } else if (static_cast<unsigned int>(res) >= buffer_sz) {
        // output was truncated
        res = buffer_sz - 1;
    }

    for (int i = res - 1; i >= 0 && isspace(buffer[i]); --i) {
        buffer[i] = '\0';
    }

    if (level == LIBBPF_WARN) {
        logger.warn(buffer);
    } else if (level == LIBBPF_INFO) {
        logger.info(buffer);
    } else if (level == LIBBPF_DEBUG) {
        logger.debug(buffer);
    }

    return res;
}

#define FN(reason) [SKB_DROP_REASON_##reason] = #reason,
static const char *const drop_reasons[] = {DEFINE_DROP_REASON(FN, FN)};

int DropTrace::ringbuf_handler(void *ctx,
                               void *data,
                               [[maybe_unused]] size_t size) {
    logger.debug("ringbuf_handler --");
    struct drop_data *d = static_cast<struct drop_data *>(data);
    [[maybe_unused]] DropTrace *this_dt = static_cast<DropTrace *>(ctx);

    constexpr int buffer_sz = 2048;
    static char buffer[buffer_sz];

    snprintf(buffer, buffer_sz,
             "[%.9f] dev: %d, iif: %d, ip len: %d, ip proto: %d, reason: %s",
             (double)d->tstamp / 1e9, d->ifindex, d->ingress_ifindex,
             d->tot_len, d->ip_proto, drop_reasons[d->reason]);
    logger.debug(buffer);
    return 0;
}

DropTrace &DropTrace::get() {
    static DropTrace instance;
    return instance;
}

void DropTrace::init() {
    teardown();
    _enabled = true;
}

void DropTrace::teardown() {
    stop_listening();
    stop();
    _enabled = false;
}

void DropTrace::start() {
    if (!_enabled) {
        return;
    }

    _bpf = droptrace_bpf__open();

    if (!_bpf) {
        logger.error("Failed to open BPF program", errno);
    }

    if (_bpf->load(_bpf)) {
        logger.error("Failed to load and verify BPF program", errno);
    }
}

void DropTrace::stop() {
    if (!_enabled) {
        return;
    }

    if (_bpf) {
        _bpf->destroy(_bpf); // destroy will also detach
        _bpf = nullptr;
    }
}

void DropTrace::start_listening_for(const Packet &pkt) {
    if (!_enabled || !_bpf) {
        return;
    }

    _target_pkt_data = pkt.to_drop_data();

    if (bpf_map__update_elem(_bpf->maps.target_packet, &_target_pkt_index,
                             sizeof(_target_pkt_index), &_target_pkt_data,
                             sizeof(_target_pkt_data), BPF_NOEXIST) < 0) {
        logger.error("Failed to update target packet", errno);
    }

    if (_bpf->attach(_bpf)) {
        logger.error("Failed to attach BPF program", errno);
    }

    _ringbuf = ring_buffer__new(bpf_map__fd(_bpf->maps.events),
                                DropTrace::ringbuf_handler, this, nullptr);

    if (!_ringbuf) {
        logger.error("Failed to create ring buffer", errno);
    }
}

uint64_t DropTrace::get_drop_ts(chrono::microseconds timeout) {
    if (!_enabled) {
        return 0;
    }

    int timeout_ms = (timeout.count() < 0)
                         ? -1
                         : duration_cast<chrono::milliseconds>(timeout).count();
    int res = ring_buffer__poll(_ringbuf, timeout_ms);

    if (res < 0) {
        if (res == -EINTR) {
            logger.debug("ring_buffer__poll is interrupted by signal (EINTR)");
        } else {
            logger.error("ring_buffer__poll", errno);
        }
    } else {
        logger.debug("Consumed " + to_string(res) + " ringbuf records");
    }

    // TODO: Fetch the drop ts from the callback

    return 0;
}

void DropTrace::stop_listening() {
    if (!_enabled) {
        return;
    }

    if (_ringbuf) {
        ring_buffer__free(_ringbuf);
        _ringbuf = nullptr;
    }

    if (_bpf) {
        _bpf->detach(_bpf);

        if (bpf_map__delete_elem(_bpf->maps.target_packet, &_target_pkt_index,
                                 sizeof(_target_pkt_index), 0) < 0) {
            logger.error("Failed to delete target packet", errno);
        }
    }

    memset(&_target_pkt_data, 0, sizeof(_target_pkt_data));
}
