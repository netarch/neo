#include "../build/vmlinux.h"

#include <linux/string.h>

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

#include "droptrace.h"

// packet.hpp
#define ID_ETH_ADDR                                                            \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }

char LICENSE[] SEC("license") = "Dual MIT/GPL";

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 10 * 1024);
} events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct drop_data);
} target_packet SEC(".maps");

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
    BPF_CORE_READ_INTO(&th, skb, transport_header);
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

static inline struct net *dev_net(const struct net_device *dev) {
    struct net *net = NULL;
    BPF_CORE_READ_INTO(&net, dev, nd_net.net);
    return net;
}

static inline struct net *sock_net(const struct sock *sk) {
    struct net *net = NULL;
    BPF_CORE_READ_INTO(&net, sk, __sk_common.skc_net.net);
    return net;
}

static inline struct dst_entry *skb_dst(const struct sk_buff *skb) {
    unsigned long refdst;
    BPF_CORE_READ_INTO(&refdst, skb, _skb_refdst);
    struct dst_entry *dst = (struct dst_entry *)(refdst & SKB_DST_PTRMASK);

    if (dst) {
        unsigned short flags;
        BPF_CORE_READ_INTO(&flags, dst, flags);
        if (flags & DST_METADATA) {
            return NULL;
        }
    }

    return dst;
}

static inline struct net *skb_net(const struct sk_buff *skb) {
    struct net_device *dev = NULL;
    BPF_CORE_READ_INTO(&dev, skb, dev);
    if (dev) {
        return dev_net(dev);
    }

    struct sock *sk = NULL;
    BPF_CORE_READ_INTO(&sk, skb, sk);
    if (sk) {
        return sock_net(sk);
    }

    struct dst_entry *dst = skb_dst(skb);
    if (dst) {
        struct net_device *dst_dev = NULL;
        BPF_CORE_READ_INTO(&dst_dev, dst, dev);
        if (dst_dev) {
            return dev_net(dst_dev);
        }
    }

    return NULL;
}

static inline unsigned int skb_netns_ino(const struct sk_buff *skb) {
    unsigned int ino = 0;
    struct net *net = skb_net(skb);

    if (net) {
        BPF_CORE_READ_INTO(&ino, net, ns.inum);
    }

    return ino;
}

SEC("tracepoint/skb/kfree_skb")
int tracepoint__kfree_skb(struct trace_event_raw_kfree_skb *args) {
    struct drop_data *target_pkt = NULL;
    u32 target_pkt_key = 0;
    target_pkt = (struct drop_data *)bpf_map_lookup_elem(&target_packet,
                                                         &target_pkt_key);

    // If there is no target packet, skip event
    if (!target_pkt) {
        return 0;
    }

    // Get sk_buff and skip event if the sk_buff is empty
    struct sk_buff *skb = NULL;
    unsigned int skb_len = 0;
    BPF_CORE_READ_INTO(&skb, args, skbaddr);
    BPF_CORE_READ_INTO(&skb_len, skb, len);

    if (skb_len == 0) {
        return 0;
    }

    // Reserve drop data for this event
    struct drop_data *data = NULL;
    data = (struct drop_data *)bpf_ringbuf_reserve(&events,
                                                   sizeof(struct drop_data), 0);
    if (!data) {
        return 0;
    }

    // Auxiliary
    data->tstamp = bpf_ktime_get_tai_ns();

    // L1
    BPF_CORE_READ_INTO(&data->ifindex, skb, dev, ifindex);
    BPF_CORE_READ_INTO(&data->ingress_ifindex, skb, skb_iif);
    data->netns_ino = skb_netns_ino(skb);

    if (!data->ingress_ifindex || !data->netns_ino) {
        goto discard;
    } else if (target_pkt->ingress_ifindex && target_pkt->netns_ino) {
        if (data->ingress_ifindex != target_pkt->ingress_ifindex ||
            data->netns_ino != target_pkt->netns_ino) {
            goto discard;
        }
    }

    // L2
    if (skb_mac_header_was_set(skb)) {
        uint8_t id_mac[6] = ID_ETH_ADDR;
        struct ethhdr *eth = eth_hdr(skb);
        BPF_CORE_READ_INTO(&data->eth_dst_addr, eth, h_dest);
        BPF_CORE_READ_INTO(&data->eth_src_addr, eth, h_source);
        BPF_CORE_READ_INTO(&data->eth_proto, eth, h_proto);

        if (data->eth_proto != target_pkt->eth_proto ||
            (memcmp(data->eth_dst_addr, id_mac, 6) != 0 &&
             memcmp(data->eth_src_addr, id_mac, 6) != 0)) {
            goto discard;
        }
    } else {
        goto discard;
    }

    // L3
    if (skb_network_header_len(skb) > 0) {
        struct iphdr *nh = ip_hdr(skb);
        BPF_CORE_READ_INTO(&data->tot_len, nh, tot_len);
        BPF_CORE_READ_INTO(&data->ip_proto, nh, protocol);
        BPF_CORE_READ_INTO(&data->saddr, nh, saddr);
        BPF_CORE_READ_INTO(&data->daddr, nh, daddr);

        if (data->ip_proto != target_pkt->ip_proto ||
            data->saddr != target_pkt->saddr ||
            data->daddr != target_pkt->daddr) {
            goto discard;
        }
    } else {
        goto discard;
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
                goto discard;
            }
        } else {
            goto discard;
        }

        if (data->ip_proto == IPPROTO_TCP || data->ip_proto == IPPROTO_UDP) {
            if (data->transport.sport != target_pkt->transport.sport ||
                data->transport.dport != target_pkt->transport.dport ||
                data->transport.seq != target_pkt->transport.seq ||
                data->transport.ack != target_pkt->transport.ack) {
                goto discard;
            }
        } else { // data->ip_proto == IPPROTO_ICMP
            if (data->icmp.icmp_type != target_pkt->icmp.icmp_type ||
                data->icmp.icmp_echo_id != target_pkt->icmp.icmp_echo_id ||
                data->icmp.icmp_echo_seq != target_pkt->icmp.icmp_echo_seq) {
                goto discard;
            }
        }
    }

    bpf_ringbuf_submit(data, BPF_RB_FORCE_WAKEUP);
    return 0;

discard:
    bpf_ringbuf_discard(data, 0);
    return 0;
}
