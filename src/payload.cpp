#include "payload.hpp"

#include <cstring>
#include <functional>

#include "eqclass.hpp"
#include "packet.hpp"
#include "lib/hash.hpp"
#include "model-access.hpp"
#include "model.h"

Payload::Payload(const std::string& pl)
{
    size = pl.size() * sizeof(char) / sizeof(uint8_t);
    if (size == 0) {
        buffer = nullptr;
    } else {
        buffer = new uint8_t[size];
        memcpy(buffer, pl.c_str(), size);
    }
}

Payload::~Payload()
{
    delete [] buffer;
}

uint8_t *Payload::get() const
{
    return buffer;
}

uint32_t Payload::get_size() const
{
    return size;
}

bool operator==(const Payload& a, const Payload& b)
{
    if (a.size == b.size) {
        return memcmp(a.buffer, b.buffer, a.size * sizeof(uint8_t)) == 0;
    }
    return false;
}

/******************************************************************************/

PayloadKey::PayloadKey(State *state)
{
    this->ec = get_ec(state);
    this->proto_state = get_proto_state(state);
}

bool operator==(const PayloadKey& a, const PayloadKey& b)
{
    if (a.ec == b.ec && a.proto_state == b.proto_state) {
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

size_t PayloadHash::operator()(Payload *const& payload) const
{
    return hash::hash(payload->get(), payload->get_size() * sizeof(uint8_t));
}

bool PayloadEq::operator()(Payload *const& a, Payload *const& b) const
{
    return *a == *b;
}

/******************************************************************************/

size_t PayloadKeyHash::operator()(const PayloadKey& key) const
{
    size_t value = 0;
    hash::hash_combine(value, std::hash<EqClass *>()(key.ec));
    hash::hash_combine(value, std::hash<uint8_t>()(key.proto_state));
    return value;
}

/******************************************************************************/

PayloadMgr::~PayloadMgr()
{
    for (Payload *payload : all_payloads) {
        delete payload;
    }
}

PayloadMgr& PayloadMgr::get()
{
    static PayloadMgr instance;
    return instance;
}

Payload *PayloadMgr::get_payload(State *state)
{
    PayloadKey key(state);

    auto it = tbl.find(key);
    if (it != tbl.end()) {
        return it->second;
    }

    std::string pl_content;

    if (key.proto_state == PS_TCP_L7_REQ) {
        pl_content = "GET / HTTP/1.1\r\n"
                     "Host: " + key.ec->representative_addr().to_string()
                     + ":" + std::to_string(get_dst_port(state)) + "\r\n"
                     "\r\n";
    } else if (key.proto_state == PS_TCP_L7_REP) {
        std::string http = "<!DOCTYPE html>"
                           "<html>"
                           "<head><title>Reply</title></head>"
                           "<body>Reply</body>"
                           "</html>\r\n";
        pl_content = "HTTP/1.1 200 OK\r\n"
                     "Server: plankton\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: " + std::to_string(http.size()) + "\r\n"
                     "\r\n";
    }

    Payload *payload = new Payload(pl_content);
    auto res = all_payloads.insert(payload);
    if (!res.second) {
        delete payload;
        payload = *(res.first);
    }
    tbl.emplace(key, payload);
    return payload;
}
