#include "emu-pkt-key.hpp"

EmuPktKey::EmuPktKey(const IPv4Address &addr, uint16_t port)
    : ip(addr), port(port) {}

std::string EmuPktKey::to_string() const {
    return ip.to_string() + ":" + std::to_string(port);
}

IPv4Address EmuPktKey::get_dst_ip() const {
    return this->ip;
}

uint16_t EmuPktKey::get_dst_port() const {
    return this->port;
}
