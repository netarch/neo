#include "lib/ip.hpp"

#include <cstdio>

#include "logger.hpp"

IPv4Address::IPv4Address(const std::string &ips) {
    int parsed, r, oct[4];
    r = sscanf(ips.c_str(), "%d.%d.%d.%d%n", oct, oct + 1, oct + 2, oct + 3,
               &parsed);
    if (r != 4 || parsed != (int)ips.size()) {
        logger.error("Failed to parse IP: " + ips);
    }
    value = 0;
    for (int i = 0; i < 4; ++i) {
        if (oct[i] < 0 || oct[i] > 255) {
            logger.error("Invalid IP octet: " + std::to_string(oct[i]));
        }
        value = (value << 8) + oct[i];
    }
}

IPv4Address::IPv4Address(const char *ips) : IPv4Address(std::string(ips)) {}

IPv4Address::IPv4Address(uint32_t ip) : value(ip) {}

std::string IPv4Address::to_string() const {
    return std::to_string((value >> 24) & 255) + "." +
           std::to_string((value >> 16) & 255) + "." +
           std::to_string((value >> 8) & 255) + "." +
           std::to_string((value) & 255);
}

size_t IPv4Address::length() const {
    return 32;
}

uint32_t IPv4Address::get_value() const {
    return value;
}

IPv4Address &IPv4Address::operator+=(const IPv4Address &rhs) {
    value += rhs.value;
    return *this;
}

IPv4Address &IPv4Address::operator-=(const IPv4Address &rhs) {
    value -= rhs.value;
    return *this;
}

IPv4Address &IPv4Address::operator&=(const IPv4Address &rhs) {
    value &= rhs.value;
    return *this;
}

IPv4Address &IPv4Address::operator|=(const IPv4Address &rhs) {
    value |= rhs.value;
    return *this;
}

IPv4Address IPv4Address::operator+(const IPv4Address &rhs) const {
    return IPv4Address(value + rhs.value);
}

int IPv4Address::operator-(const IPv4Address &rhs) const {
    return (int)(value - rhs.value);
}

IPv4Address IPv4Address::operator&(const IPv4Address &rhs) const {
    return IPv4Address(value & rhs.value);
}

uint32_t IPv4Address::operator&(uint32_t rhs) const {
    return value & rhs;
}

IPv4Address IPv4Address::operator|(const IPv4Address &rhs) const {
    return IPv4Address(value | rhs.value);
}

uint32_t IPv4Address::operator|(uint32_t rhs) const {
    return value | rhs;
}

bool operator<(const IPv4Address &a, const IPv4Address &b) {
    return a.value < b.value;
}

bool operator<=(const IPv4Address &a, const IPv4Address &b) {
    return a.value <= b.value;
}

bool operator>(const IPv4Address &a, const IPv4Address &b) {
    return a.value > b.value;
}

bool operator>=(const IPv4Address &a, const IPv4Address &b) {
    return a.value >= b.value;
}

bool operator==(const IPv4Address &a, const IPv4Address &b) {
    return a.value == b.value;
}

bool operator!=(const IPv4Address &a, const IPv4Address &b) {
    return a.value != b.value;
}
