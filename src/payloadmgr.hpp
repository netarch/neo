#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "payload.hpp"

class EqClass;
class Model;
class Packet;

class PayloadKey {
private:
    EqClass *dst_ip_ec;
    uint16_t dst_port;
    uint8_t proto_state;

    friend class PayloadMgr;
    friend class PayloadKeyHash;
    friend bool operator==(const PayloadKey &, const PayloadKey &);

public:
    PayloadKey(const Model &);
};

bool operator==(const PayloadKey &, const PayloadKey &);

class PayloadKeyHash {
public:
    size_t operator()(const PayloadKey &) const;
};

class PayloadMgr {
private:
    std::unordered_set<Payload *, PayloadHash, PayloadEq> all_payloads;
    std::unordered_map<PayloadKey, Payload *, PayloadKeyHash> state_to_pl_map;

    PayloadMgr() = default;

public:
    // Disable the copy constructor and the copy assignment operator
    PayloadMgr(const PayloadMgr &)            = delete;
    PayloadMgr &operator=(const PayloadMgr &) = delete;
    ~PayloadMgr();

    static PayloadMgr &get();

    void reset();
    Payload *get_payload_from_model();
    Payload *get_payload(uint8_t *, size_t);
    // Concatenate the payloads of two packets
    Payload *get_payload(const Packet &, const Packet &);
};
