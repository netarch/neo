#include "vmlinux.h"

#include "droptrace.h"

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "Dual MIT/GPL";

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024 /* 256 KB */);
} events SEC(".maps");

static inline bool skb_mac_header_was_set(const struct sk_buff *skb) {
    u16 mac_header;
    BPF_CORE_READ_INTO(&mac_header, skb, mac_header);
    return mac_header != (typeof(mac_header))~0U;
}

static inline unsigned char *skb_mac_header(const struct sk_buff *skb) {
    u16 mac_header;
    unsigned char *head;
    BPF_CORE_READ_INTO(&mac_header, skb, mac_header);
    BPF_CORE_READ_INTO(&head, skb, head);
    return head + mac_header;
}

static inline u32 skb_network_header_len(const struct sk_buff *skb) {
    u16 th, nh;
    BPF_CORE_READ_INTO(&th, skb, transport_header);
    BPF_CORE_READ_INTO(&nh, skb, network_header);
    return th - nh;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb) {
    u16 nh;
    unsigned char *head;
    BPF_CORE_READ_INTO(&head, skb, head);
    BPF_CORE_READ_INTO(&nh, skb, network_header);
    return head + nh;
}

static inline bool skb_transport_header_was_set(const struct sk_buff *skb) {
    u16 th;
    BPF_CORE_READ_INTO(&th, skb, transport_header);
    return th != (typeof(th))~0U;
}

static inline unsigned char *skb_transport_header(const struct sk_buff *skb) {
    u16 th;
    unsigned char *head;
    BPF_CORE_READ_INTO(&head, skb, head);
    BPF_CORE_READ_INTO(&th, skb, network_header);
    return head + th;
}

static inline struct ethhdr *eth_hdr(const struct sk_buff *skb) {
    return (struct ethhdr *)skb_mac_header(skb);
}

static inline struct iphdr *ip_hdr(const struct sk_buff *skb) {
    return (struct iphdr *)skb_network_header(skb);
}

static inline struct tcphdr *tcp_hdr(const struct sk_buff *skb) {
    return (struct tcphdr *)skb_transport_header(skb);
}

static inline struct udphdr *udp_hdr(const struct sk_buff *skb) {
    return (struct udphdr *)skb_transport_header(skb);
}

static inline struct icmphdr *icmp_hdr(const struct sk_buff *skb) {
    return (struct icmphdr *)skb_transport_header(skb);
}

SEC("tracepoint/skb/kfree_skb")
int tracepoint__kfree_skb(struct trace_event_raw_kfree_skb *args) {
    struct drop_data *data = NULL;
    struct sk_buff *skb = NULL;
    unsigned short protocol = 0;
    unsigned int len = 0;
    int skb_iif = 0;

    BPF_CORE_READ_INTO(&skb, args, skbaddr);
    BPF_CORE_READ_INTO(&protocol, args, protocol);
    BPF_CORE_READ_INTO(&len, skb, len);
    BPF_CORE_READ_INTO(&skb_iif, skb, skb_iif);

    if (len == 0 || !skb_iif || protocol != ETH_P_IP) {
        return 0;
    }

    data = bpf_ringbuf_reserve(&events, sizeof(struct drop_data), 0);
    if (!data) {
        return 0;
    }

    // L1
    BPF_CORE_READ_INTO(&data->ifindex, skb, dev, ifindex);
    data->ingress_ifindex = skb_iif;
    // auxiliary
    BPF_CORE_READ_INTO(&data->location, args, location);
    data->tstamp = bpf_ktime_get_tai_ns();
    BPF_CORE_READ_INTO(&data->reason, args, reason);

    // L2
    if (skb_mac_header_was_set(skb)) {
        struct ethhdr *eth = eth_hdr(skb);
        BPF_CORE_READ_INTO(&data->eth_dst_addr, eth, h_dest);
        BPF_CORE_READ_INTO(&data->eth_src_addr, eth, h_source);
    }

    data->eth_proto = protocol;

    // L3
    if (data->eth_proto == ETH_P_IP && skb_network_header_len(skb) > 0) {
        struct iphdr *nh = ip_hdr(skb);
        BPF_CORE_READ_INTO(&data->tot_len, nh, tot_len);
        BPF_CORE_READ_INTO(&data->ip_proto, nh, protocol);
        BPF_CORE_READ_INTO(&data->saddr, nh, saddr);
        BPF_CORE_READ_INTO(&data->daddr, nh, daddr);
    }

    // L4
    if (skb_transport_header_was_set(skb)) {
        if (data->ip_proto == IPPROTO_TCP) {
            struct tcphdr *th = tcp_hdr(skb);
            BPF_CORE_READ_INTO(&data->transport.sport, th, source);
            BPF_CORE_READ_INTO(&data->transport.dport, th, dest);
            BPF_CORE_READ_INTO(&data->transport.seq, th, seq);
            BPF_CORE_READ_INTO(&data->transport.ack, th, ack_seq);
        } else if (data->ip_proto == IPPROTO_UDP) {
            struct udphdr *uh = udp_hdr(skb);
            BPF_CORE_READ_INTO(&data->transport.sport, uh, source);
            BPF_CORE_READ_INTO(&data->transport.dport, uh, dest);
            data->transport.seq = 0;
            data->transport.ack = 0;
        } else if (data->ip_proto == IPPROTO_ICMP) {
            struct icmphdr *ih = icmp_hdr(skb);
            BPF_CORE_READ_INTO(&data->icmp.icmp_type, ih, type);
            if (data->icmp.icmp_type == ICMP_ECHOREPLY ||
                data->icmp.icmp_type == ICMP_ECHO) {
                BPF_CORE_READ_INTO(&data->icmp.icmp_echo_id, ih, un.echo.id);
                BPF_CORE_READ_INTO(&data->icmp.icmp_echo_seq, ih,
                                   un.echo.sequence);
            } else {
                data->icmp.icmp_echo_id = 0;
                data->icmp.icmp_echo_seq = 0;
            }
        } else {
            goto discard;
        }
    }

    bpf_ringbuf_submit(data, BPF_RB_FORCE_WAKEUP);
    return 0;

discard:
    bpf_ringbuf_discard(data, 0);
    return 0;
}
