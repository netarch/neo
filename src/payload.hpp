#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

class EqClass;
class Packet;
struct State;

class Payload {
private:
    uint8_t *buffer;
    uint32_t size;

    friend class PayloadMgr;
    friend bool operator==(const Payload &, const Payload &);

public:
    Payload() = delete;
    Payload(const Payload &) = delete;
    Payload(Payload &&) = default;
    Payload(const std::string &);
    Payload(uint8_t *, size_t);
    Payload(const Payload *const, const Payload *const);
    ~Payload();

    Payload &operator=(const Payload &) = delete;
    Payload &operator=(Payload &&) = default;

    uint8_t *get() const;
    uint32_t get_size() const;
};

bool operator==(const Payload &, const Payload &);

class PayloadKey {
private:
    EqClass *dst_ip_ec;
    uint16_t dst_port;
    uint8_t proto_state;

    friend class PayloadMgr;
    friend class PayloadKeyHash;
    friend bool operator==(const PayloadKey &, const PayloadKey &);

public:
    PayloadKey(State *);
};

bool operator==(const PayloadKey &, const PayloadKey &);

class PayloadHash {
public:
    size_t operator()(Payload *const &) const;
};

class PayloadEq {
public:
    bool operator()(Payload *const &, Payload *const &) const;
};

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
    PayloadMgr(const PayloadMgr &) = delete;
    PayloadMgr &operator=(const PayloadMgr &) = delete;
    ~PayloadMgr();

    static PayloadMgr &get();

    Payload *get_payload(State *);
    Payload *get_payload(uint8_t *, size_t);
    // Concatenate the payloads of two packets
    Payload *get_payload(const Packet &, const Packet &);
};
