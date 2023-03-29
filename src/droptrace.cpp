#include "droptrace.hpp"

#include <cerrno>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "droptrace.h"
#include "droptrace.skel.h"
#include "logger.hpp"

using namespace std;

// static int libbpf_print_fn(enum libbpf_print_level level,
//                            const char *format,
//                            va_list args) {
//     if (level >= LIBBPF_DEBUG) {
//         return 0;
//     }
//     return vfprintf(stderr, format, args);
// }

DropTrace::DropTrace() : _bpf(nullptr), _ringbuf(nullptr) {
    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    // libbpf_set_print(libbpf_print_fn);
}

DropTrace::~DropTrace() {
    ring_buffer__free(_ringbuf);
    droptrace_bpf__destroy(_bpf);
}

// #define FN(reason) [SKB_DROP_REASON_##reason] = #reason,
// static const char *const drop_reasons[] = {DEFINE_DROP_REASON(FN, FN)};

int DropTrace::ringbuf_handler(void *ctx __attribute__((unused)),
                               void *data __attribute__((unused)),
                               size_t size __attribute__((unused))) {
    logger.debug("ringbuf_handler");

    // struct drop_data *d = data;
    // printf("[%.9f] dev: %d, iif: %d, ip len: %d, ip proto: %d, reason: %s\n",
    //        (double)d->tstamp / 1e9, d->ifindex, d->ingress_ifindex,
    //        d->tot_len, d->ip_proto, drop_reasons[d->reason]);

    return 0;
}

DropTrace &DropTrace::get() {
    static DropTrace instance;
    return instance;
}

void DropTrace::init() {
    _bpf = droptrace_bpf__open();

    if (!_bpf) {
        logger.error("Failed to open BPF program");
    }

    if (droptrace_bpf__load(_bpf) != 0) {
        logger.error("Failed to load and verify BPF program", errno);
    }

    if (droptrace_bpf__attach(_bpf) != 0) {
        logger.error("Failed to attach BPF program");
    }

    _ringbuf = ring_buffer__new(bpf_map__fd(_bpf->maps.events), ringbuf_handler,
                                this, nullptr);

    if (!_ringbuf) {
        logger.error("Failed to create ring buffer");
    }
}

// int main(void) {
//     while (1) {
//         // ring_buffer__epoll_fd
//         int err = ring_buffer__poll(rb, 100);
//         if (err == -EINTR) {
//             err = 0;
//             break;
//         } else if (err < 0) {
//             perror("ring_buffer__poll");
//             break;
//         }
//     }
// }
