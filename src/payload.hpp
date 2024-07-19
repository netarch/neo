#pragma once

#include <cstdint>
#include <string>

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

    decltype(buffer) get() const { return buffer; }
    decltype(size) get_size() const { return size; }
};

bool operator==(const Payload &, const Payload &);

class PayloadHash {
public:
    size_t operator()(Payload *const &) const;
};

class PayloadEq {
public:
    bool operator()(Payload *const &, Payload *const &) const;
};
