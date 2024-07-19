#include "payloadmgr.hpp"

#include <cstring>
#include <functional>

#include "eqclass.hpp"
#include "lib/hash.hpp"
#include "model-access.hpp"
#include "packet.hpp"
#include "protocols.hpp"

using namespace std;

PayloadKey::PayloadKey(const Model &model) {
    this->dst_ip_ec = model.get_dst_ip_ec();
    this->dst_port = model.get_dst_port();
    this->proto_state = model.get_proto_state();
}

bool operator==(const PayloadKey &a, const PayloadKey &b) {
    if (a.dst_ip_ec == b.dst_ip_ec && a.dst_port == b.dst_port &&
        a.proto_state == b.proto_state) {
        return true;
    } else {
        return false;
    }
}

size_t PayloadKeyHash::operator()(const PayloadKey &key) const {
    size_t value = 0;
    ::hash::hash_combine(value, std::hash<EqClass *>()(key.dst_ip_ec));
    ::hash::hash_combine(value, std::hash<uint16_t>()(key.dst_port));
    ::hash::hash_combine(value, std::hash<uint8_t>()(key.proto_state));
    return value;
}

PayloadMgr::~PayloadMgr() {
    reset();
}

PayloadMgr &PayloadMgr::get() {
    static PayloadMgr instance;
    return instance;
}

void PayloadMgr::reset() {
    for (Payload *pl : this->all_payloads) {
        delete pl;
    }

    this->all_payloads.clear();
    this->state_to_pl_map.clear();
}

Payload *PayloadMgr::get_payload_from_model() {
    PayloadKey key(model);

    if (key.proto_state != PS_TCP_L7_REQ && key.proto_state != PS_TCP_L7_REP) {
        return nullptr;
    }

    auto it = this->state_to_pl_map.find(key);
    if (it != this->state_to_pl_map.end()) {
        return it->second;
    }

    std::string pl_content;

    if (key.proto_state == PS_TCP_L7_REQ) {
        pl_content = "GET / HTTP/1.1\r\n"
                     "Host: " +
                     key.dst_ip_ec->representative_addr().to_string() + ":" +
                     std::to_string(key.dst_port) +
                     "\r\n"
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
                     "Content-Length: " +
                     std::to_string(http.size()) +
                     "\r\n"
                     "\r\n";
    } else {
        return nullptr;
    }

    Payload *payload = new Payload(pl_content);
    auto res = this->all_payloads.insert(payload);
    if (!res.second) {
        delete payload;
        payload = *(res.first);
    }
    this->state_to_pl_map.emplace(key, payload);
    return payload;
}

Payload *PayloadMgr::get_payload(uint8_t *data, size_t len) {
    if (len == 0) {
        return nullptr;
    }

    Payload *payload = new Payload(data, len);
    auto res = this->all_payloads.insert(payload);
    if (!res.second) {
        delete payload;
        payload = *(res.first);
    }
    return payload;
}

Payload *PayloadMgr::get_payload(const Packet &a, const Packet &b) {
    // Concatenate the payloads of two packets
    Payload *payload = new Payload(a.get_payload(), b.get_payload());
    auto res = this->all_payloads.insert(payload);
    if (!res.second) {
        delete payload;
        payload = *(res.first);
    }
    return payload;
}
