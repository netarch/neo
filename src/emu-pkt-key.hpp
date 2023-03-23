#pragma once

#include <cstdint>
#include <string>

#include "lib/hash.hpp"
#include "lib/ip.hpp"

/**
 * An EmuPktKey object is intended as key for _seq_offsets and _port_offsets
 * mappings (i.e., those that are not part of the state vector). Such object
 * uniquely identifies a concrete connection associated with the emulation from
 * the given packet, so that we don't need to update the model state to update
 * or retrieve the offsets, and that the emulation can handle these tasks
 * related to interacting with the real world on its own.
 */
class EmuPktKey {
private:
    IPv4Address ip; // IP address of the opposite endpoint
    uint16_t port;  // port of the opposite endpoint

    friend class EmuPktKeyHash;

public:
    EmuPktKey(const IPv4Address &, uint16_t);
    EmuPktKey(const EmuPktKey &) = default;
    EmuPktKey(EmuPktKey &&) = default;

    bool operator==(const EmuPktKey &) const = default;

    std::string to_string() const;
    IPv4Address get_dst_ip() const;
    uint16_t get_dst_port() const;
};

namespace std {

template <>
struct hash<EmuPktKey> {
    size_t operator()(const EmuPktKey &key) const {
        size_t value = 0;
        ::hash::hash_combine(value, std::hash<IPv4Address>()(key.get_dst_ip()));
        ::hash::hash_combine(value, std::hash<uint16_t>()(key.get_dst_port()));
        return value;
    }
};

} // namespace std
