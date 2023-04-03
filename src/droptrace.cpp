#include "droptrace.hpp"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "droptrace.h"
#include "droptrace.skel.h"
#include "logger.hpp"

using namespace std;

DropTrace::DropTrace()
    : DropDetection(), _bpf(nullptr), _ringbuf(nullptr), _target_pkt_key(0),
      _drop_ts(0) {
    memset(&_target_pkt, 0, sizeof(_target_pkt));
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

int DropTrace::ringbuf_handler(void *ctx,
                               void *data,
                               [[maybe_unused]] size_t size) {
    struct drop_data *d = static_cast<struct drop_data *>(data);
    DropTrace *this_dt = static_cast<DropTrace *>(ctx);
    this_dt->_drop_ts = d->tstamp;

#ifdef ENABLE_DEBUG
    // Print out debugging messages
    {
        constexpr int buffer_sz = 2048;
        static char buffer[buffer_sz];
        auto src_ip = IPv4Address(ntohl(d->saddr)).to_string();
        auto dst_ip = IPv4Address(ntohl(d->daddr)).to_string();
        int nwrites =
            snprintf(buffer, buffer_sz,
                     "ts: %llu, dev: %d, iif: %d, netns_ino: %u, ip_proto: %d, "
                     "src_ip: %s, dst_ip: %s",
                     d->tstamp, d->ifindex, d->ingress_ifindex, d->netns_ino,
                     d->ip_proto, src_ip.c_str(), dst_ip.c_str());

        if (d->ip_proto == IPPROTO_TCP || d->ip_proto == IPPROTO_UDP) {
            u16 sport = ntohs(d->transport.sport);
            u16 dport = ntohs(d->transport.dport);
            u32 seq = ntohl(d->transport.seq);
            u32 ack = ntohl(d->transport.ack);
            snprintf(buffer + nwrites, buffer_sz - nwrites,
                     ", sport: %u, dport: %u, seq: %u, ack: %u", sport, dport,
                     seq, ack);
        } else if (d->ip_proto == IPPROTO_ICMP) {
            u16 echo_id = ntohs(d->icmp.icmp_echo_id);
            u16 echo_seq = ntohs(d->icmp.icmp_echo_seq);
            snprintf(buffer + nwrites, buffer_sz - nwrites,
                     ", icmp_type: %u, echo_id: %u, echo_seq: %u",
                     d->icmp.icmp_type, echo_id, echo_seq);
        } else {
            logger.error("Invalid ip_proto");
        }

        logger.debug(buffer);
    }
#endif

    return 0;
}

DropTrace &DropTrace::get() {
    static DropTrace instance;
    return instance;
}

void DropTrace::init() {
    teardown();
    DropDetection::init();
}

void DropTrace::teardown() {
    stop();
    DropDetection::teardown();
}

void DropTrace::start() {
    if (!_enabled) {
        return;
    }

    if (_bpf) {
        logger.error("Another BPF progam is already loaded");
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

    if (_ringbuf) {
        ring_buffer__free(_ringbuf);
        _ringbuf = nullptr;
    }

    if (_bpf) {
        _bpf->destroy(_bpf); // destroy will also detach
        _bpf = nullptr;
    }

    memset(&_target_pkt, 0, sizeof(_target_pkt));
    _drop_ts = 0;
}

void DropTrace::start_listening_for(const Packet &pkt, Driver *driver) {
    if (!_enabled || !_bpf) {
        return;
    }

    if (pkt.empty()) {
        logger.error("Empty target packet");
    }

    if (_ringbuf) {
        logger.error("Ring buffer already opened");
    }

    _target_pkt = pkt.to_drop_data(driver);
    _drop_ts = 0;

    if (bpf_map__update_elem(_bpf->maps.target_packet, &_target_pkt_key,
                             sizeof(_target_pkt_key), &_target_pkt,
                             sizeof(_target_pkt), BPF_NOEXIST) < 0) {
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

    if (res < 0 && res != -EINTR) {
        logger.error("ring_buffer__poll", errno);
    }

    return _drop_ts;
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

        if (bpf_map__delete_elem(_bpf->maps.target_packet, &_target_pkt_key,
                                 sizeof(_target_pkt_key), 0) < 0) {
            logger.error("Failed to delete target packet", errno);
        }
    }

    memset(&_target_pkt, 0, sizeof(_target_pkt));
    _drop_ts = 0;
}
