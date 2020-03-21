#pragma once

#include <libnet.h>

#include "packet.hpp"

class Net
{
private:
    libnet_t *l;

    Net();
    void build_tcp(const Packet& pkt, const uint8_t *src_mac,
                   const uint8_t *dst_mac) const;
    void build_icmp_echo(const Packet& pkt, const uint8_t *src_mac,
                         const uint8_t *dst_mac) const;

public:
    // Disable the copy constructor and the copy assignment operator
    Net(const Net&) = delete;
    Net& operator=(const Net&) = delete;
    ~Net();

    static Net& get();

    /*
     * Net::serialize()
     *
     * It serializes the packet and stores it in buffer.
     *
     * NOTE: the buffer should freed later by Net::free().
     */
    void serialize(uint8_t **buffer, uint32_t *buffer_size, const Packet& pkt,
                   const uint8_t *src_mac,
                   const uint8_t *dst_mac) const;
    void free(uint8_t *) const;

    std::string mac_to_str(const uint8_t *) const;
};
