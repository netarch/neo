#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "eqclass.hpp"
class State;

class Payload
{
private:
    uint8_t *buffer;
    uint32_t size;

    friend class PayloadMgr;
    friend bool operator==(const Payload&, const Payload&);

public:
    Payload() = delete;
    Payload(const Payload&) = delete;
    Payload(Payload&&) = default;
    Payload(const std::string&);
    ~Payload();

    Payload& operator=(const Payload&) = delete;
    Payload& operator=(Payload&&) = default;

    uint8_t *get() const;
    uint32_t get_size() const;
};

bool operator==(const Payload&, const Payload&);

class PayloadKey
{
private:
    EqClass *ec;
    uint8_t pkt_state;

    friend class PayloadMgr;
    friend class PayloadKeyHash;
    friend bool operator==(const PayloadKey&, const PayloadKey&);

public:
    PayloadKey(State *);
};

bool operator==(const PayloadKey&, const PayloadKey&);


struct PayloadHash {
    size_t operator()(Payload *const&) const;
};

struct PayloadEq {
    bool operator()(Payload *const&, Payload *const&) const;
};

struct PayloadKeyHash {
    size_t operator()(const PayloadKey&) const;
};


class PayloadMgr
{
private:
    std::unordered_set<Payload *, PayloadHash, PayloadEq> all_payloads;
    std::unordered_map<PayloadKey, Payload *, PayloadKeyHash> tbl;

    PayloadMgr() = default;

public:
    // Disable the copy constructor and the copy assignment operator
    PayloadMgr(const PayloadMgr&) = delete;
    PayloadMgr& operator=(const PayloadMgr&) = delete;
    ~PayloadMgr();

    static PayloadMgr& get();

    Payload *get_payload(State *, uint16_t dst_port);
};
