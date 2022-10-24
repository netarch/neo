#include "packet.hpp"

#include "eqclassmgr.hpp"
#include "interface.hpp"
#include "lib/hash.hpp"
#include "model-access.hpp"
#include "network.hpp"
#include "payload.hpp"
#include "process/process.hpp"
#include "protocols.hpp"

#include "model.h"

Packet::Packet()
    : interface(nullptr), src_ip(0U), dst_ip(0U), src_port(0), dst_port(0),
      seq(0), ack(0), proto_state(0), payload(nullptr) {}

Packet::Packet(State *state)
    : Packet() // make sure all members are initialized
{
    this->interface = ::get_ingress_intf(state);

    this->src_ip = ::get_src_ip(state);
    this->dst_ip = ::get_dst_ip_ec(state)->representative_addr();

    this->src_port = ::get_src_port(state);
    this->dst_port = ::get_dst_port(state);
    this->seq = ::get_seq(state);
    this->ack = ::get_ack(state);

    this->proto_state = ::get_proto_state(state);
    this->payload = PayloadMgr::get().get_payload(state);
}

Packet::Packet(Interface *intf,
               IPv4Address src_ip,
               IPv4Address dst_ip,
               uint16_t src_port,
               uint16_t dst_port,
               uint32_t seq,
               uint32_t ack,
               uint16_t proto_state)
    : interface(intf), src_ip(src_ip), dst_ip(dst_ip), src_port(src_port),
      dst_port(dst_port), seq(seq), ack(ack), proto_state(proto_state),
      payload(nullptr) {}

std::string Packet::to_string() const {
    int proto_state = this->proto_state;
    std::string ret;

    if (interface) {
        ret += interface->to_string() + ": ";
    }
    ret += "{ " + src_ip.to_string();
    if (PS_IS_TCP(proto_state) || PS_IS_UDP(proto_state)) {
        ret += ":" + std::to_string(src_port);
    }
    ret += " -> " + dst_ip.to_string();
    if (PS_IS_TCP(proto_state) || PS_IS_UDP(proto_state)) {
        ret += ":" + std::to_string(dst_port);
    }
    ret += " (state: " + std::to_string(proto_state) + ")";
    if (PS_IS_TCP(proto_state)) {
        ret += " [" + std::to_string(seq) + "/" + std::to_string(ack) +
               " (seq/ack)]";
    }
    if (PS_IS_TCP(proto_state) || PS_IS_UDP(proto_state)) {
        ret +=
            " payload size: " +
            (payload ? std::to_string(payload->get_size()) : std::string("0"));
    }
    ret += " }";
    return ret;
}

Interface *Packet::get_intf() const {
    return interface;
}

IPv4Address Packet::get_src_ip() const {
    return src_ip;
}

IPv4Address Packet::get_dst_ip() const {
    return dst_ip;
}

uint16_t Packet::get_src_port() const {
    return src_port;
}

uint16_t Packet::get_dst_port() const {
    return dst_port;
}

uint32_t Packet::get_seq() const {
    return seq;
}

uint32_t Packet::get_ack() const {
    return ack;
}

uint16_t Packet::get_proto_state() const {
    return proto_state;
}

Payload *Packet::get_payload() const {
    return payload;
}

void Packet::update_conn(State *state, int conn, const Network &network) const {
    int orig_conn = get_conn(state);
    set_conn(state, conn);

    EqClass *old_dst_ip_ec = ::get_dst_ip_ec(state);

    ::set_executable(state, 2);

    ::set_proto_state(state, proto_state);
    ::set_src_ip(state, src_ip.get_value());
    ::set_dst_ip_ec(state, EqClassMgr::get().find_ec(dst_ip));
    ::set_src_port(state, src_port);
    ::set_dst_port(state, dst_port);
    ::set_seq(state, seq);
    ::set_ack(state, ack);

    if (old_dst_ip_ec != ::get_dst_ip_ec(state)) {
        network.update_fib(state);
    }

    set_conn(state, orig_conn);
}

void Packet::clear() {
    interface = nullptr;
    src_ip = dst_ip = 0U;
    src_port = dst_port = 0;
    seq = ack = 0;
    proto_state = 0;
    payload = nullptr;
}

bool Packet::empty() const {
    return (interface == nullptr && src_ip.get_value() == 0 &&
            dst_ip.get_value() == 0 && src_port == 0 && dst_port == 0 &&
            seq == 0 && ack == 0);
}

bool Packet::same_flow_as(const Packet &other) const {
    return (src_ip == other.src_ip && dst_ip == other.dst_ip &&
            src_port == other.src_port && dst_port == other.dst_port &&
            PS_TO_PROTO(proto_state) == PS_TO_PROTO(other.proto_state));
}

bool Packet::same_header(const Packet &other) const {
    return (same_flow_as(other) && seq == other.seq && ack == other.ack);
}

void Packet::set_intf(Interface *intf) {
    this->interface = intf;
}

void Packet::set_src_ip(const IPv4Address &ip) {
    this->src_ip = ip;
}

void Packet::set_dst_ip(const IPv4Address &ip) {
    this->dst_ip = ip;
}

void Packet::set_src_port(uint16_t port) {
    this->src_port = port;
}

void Packet::set_dst_port(uint16_t port) {
    this->dst_port = port;
}

void Packet::set_seq_no(uint32_t n) {
    this->seq = n;
}

void Packet::set_ack_no(uint32_t n) {
    this->ack = n;
}

void Packet::set_proto_state(uint16_t s) {
    this->proto_state = s;
}

void Packet::set_payload(Payload *pl) {
    this->payload = pl;
}

bool operator==(const Packet &a, const Packet &b) {
    return (a.interface == b.interface && a.src_ip == b.src_ip &&
            a.dst_ip == b.dst_ip && a.src_port == b.src_port &&
            a.dst_port == b.dst_port && a.seq == b.seq && a.ack == b.ack &&
            a.proto_state == b.proto_state && a.payload == b.payload);
}

size_t PacketHash::operator()(Packet *const &p) const {
    size_t value = 0;
    hash::hash_combine(value, std::hash<Interface *>()(p->interface));
    hash::hash_combine(value, std::hash<IPv4Address>()(p->src_ip));
    hash::hash_combine(value, std::hash<IPv4Address>()(p->dst_ip));
    hash::hash_combine(value, std::hash<uint16_t>()(p->src_port));
    hash::hash_combine(value, std::hash<uint16_t>()(p->dst_port));
    hash::hash_combine(value, std::hash<uint32_t>()(p->seq));
    hash::hash_combine(value, std::hash<uint32_t>()(p->ack));
    hash::hash_combine(value, std::hash<uint16_t>()(p->proto_state));
    hash::hash_combine(value, std::hash<Payload *>()(p->payload));
    return value;
}

bool PacketEq::operator()(Packet *const &a, Packet *const &b) const {
    return *a == *b;
}
