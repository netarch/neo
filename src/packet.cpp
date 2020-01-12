#include "packet.hpp"

#include <cstring>

#include "pan.h"

Packet::Packet(State *state, const Policy *policy)
{
    // ingress interface
    memcpy(&interface, state->comm_state[state->comm].ingress_intf,
           sizeof(Interface *));
    // IP
    memcpy(&src_ip, state->comm_state[state->comm].src_ip, sizeof(uint32_t));
    memcpy(&dst_ip_ec, state->comm_state[state->comm].ec, sizeof(EqClass *));
    // packet state
    int pkt_state = state->comm_state[state->comm].pkt_state;
    this->pkt_state = pkt_state;

    if (PS_IS_TCP(pkt_state)) {
        // TCP
        src_port = policy->get_src_port(state);
        dst_port = policy->get_dst_port(state);
        memcpy(&seq, state->comm_state[state->comm].seq, sizeof(uint32_t));
        memcpy(&ack, state->comm_state[state->comm].ack, sizeof(uint32_t));
        payload = PayloadMgr::get().get_payload(state, dst_port);
    } else if (PS_IS_ICMP_ECHO(pkt_state)) {
        // ICMP
        src_port = 0;
        dst_port = 0;
        seq = 0;
        ack = 0;
        payload = nullptr;
    } else {
        Logger::get().err("Unsupported packet state "
                          + std::to_string(pkt_state));
    }
}

Packet::Packet(Interface *intf, const IPv4Address& src_ip,
               const IPv4Address& dst_ip, uint8_t pkt_state)
    : interface(intf), src_ip(src_ip), dst_ip(dst_ip), src_port(0), dst_port(0),
      seq(0), ack(0), pkt_state(pkt_state), payload(nullptr)
{
}

std::string Packet::to_string() const
{
    IPv4Address _dst_ip
        = PS_IS_ARP(pkt_state) ? dst_ip : dst_ip_ec->representative_addr();

    std::string ret = "{ " + src_ip.to_string();
    if (PS_IS_TCP(pkt_state)) {
        ret += ":" + std::to_string(src_port);
    }
    ret += " -> " + _dst_ip.to_string();
    if (PS_IS_TCP(pkt_state)) {
        ret += ":" + std::to_string(dst_port);
    }
    ret += " (state: " + std::to_string(pkt_state) + ")";
    if (PS_IS_TCP(pkt_state)) {
        ret +=
            " [" + std::to_string(seq) + "/" + std::to_string(ack) + " (S/A)]"
            " payload size: " +
            (payload ? std::to_string(payload->get_size()) : std::string("0"));
    }
    ret += " }";
    return ret;
}

Interface *Packet::get_intf() const
{
    return interface;
}

IPv4Address Packet::get_src_ip() const
{
    return src_ip;
}

IPv4Address Packet::get_dst_ip() const
{
    if (PS_IS_ARP(pkt_state)) {
        return dst_ip;
    } else {
        return dst_ip_ec->representative_addr();
    }
}

uint16_t Packet::get_src_port() const
{
    return src_port;
}

uint16_t Packet::get_dst_port() const
{
    return dst_port;
}

uint32_t Packet::get_seq() const
{
    return seq;
}

uint32_t Packet::get_ack() const
{
    return ack;
}

uint8_t Packet::get_pkt_state() const
{
    return pkt_state;
}

Payload *Packet::get_payload() const
{
    return payload;
}

bool operator==(const Packet& a, const Packet& b)
{
    return (a.interface == b.interface &&
            a.src_ip == b.src_ip &&
            a.dst_ip_ec == b.dst_ip_ec &&
            a.dst_ip == b.dst_ip &&
            a.src_port == b.src_port &&
            a.dst_port == b.dst_port &&
            a.seq == b.seq &&
            a.ack == b.ack &&
            a.pkt_state == b.pkt_state &&
            a.payload == b.payload);
}

size_t PacketHash::operator()(Packet *const& p) const
{
    size_t value = 0;
    ::hash::hash_combine(value, std::hash<Interface *>()(p->interface));
    ::hash::hash_combine(value, std::hash<IPv4Address>()(p->src_ip));
    ::hash::hash_combine(value, std::hash<EqClass *>()  (p->dst_ip_ec));
    ::hash::hash_combine(value, std::hash<IPv4Address>()(p->dst_ip));
    ::hash::hash_combine(value, std::hash<uint16_t>()   (p->src_port));
    ::hash::hash_combine(value, std::hash<uint16_t>()   (p->dst_port));
    ::hash::hash_combine(value, std::hash<uint32_t>()   (p->seq));
    ::hash::hash_combine(value, std::hash<uint32_t>()   (p->ack));
    ::hash::hash_combine(value, std::hash<uint8_t>()    (p->pkt_state));
    ::hash::hash_combine(value, std::hash<Payload *>()(p->payload));
    return value;
}

bool PacketEq::operator()(Packet *const& a, Packet *const& b) const
{
    return *a == *b;
}
