#include "payload.hpp"

#include <cstring>

#include "lib/hash.hpp"

using namespace std;

Payload::Payload(const string &pl) {
    this->size = pl.size() * sizeof(char) / sizeof(uint8_t);
    if (this->size == 0) {
        this->buffer = nullptr;
    } else {
        this->buffer = new uint8_t[this->size];
        memcpy(this->buffer, pl.c_str(), this->size);
    }
}

Payload::Payload(uint8_t *data, size_t len) {
    this->size = len;
    if (this->size == 0) {
        this->buffer = nullptr;
    } else {
        this->buffer = data;
    }
}

Payload::Payload(const Payload *const a, const Payload *const b) {
    uint32_t a_size = a ? a->get_size() : 0;
    uint32_t b_size = b ? b->get_size() : 0;

    this->size = a_size + b_size;

    if (this->size == 0) {
        this->buffer = nullptr;
    } else {
        this->buffer = new uint8_t[this->size];
        if (a) {
            memcpy(this->buffer, a->get(), a_size);
        }
        if (b) {
            memcpy(this->buffer + a_size, b->get(), b_size);
        }
    }
}

Payload::~Payload() {
    delete[] this->buffer;
}

bool operator==(const Payload &a, const Payload &b) {
    if (a.size == b.size) {
        return memcmp(a.buffer, b.buffer, a.size * sizeof(uint8_t)) == 0;
    }
    return false;
}

size_t PayloadHash::operator()(Payload *const &payload) const {
    return ::hash::hash(payload->get(), payload->get_size() * sizeof(uint8_t));
}

bool PayloadEq::operator()(Payload *const &a, Payload *const &b) const {
    return *a == *b;
}
