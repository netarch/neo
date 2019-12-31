#pragma once

#include <libnet.h>

#include "packet.hpp"

class Net
{
private:
    libnet_t *l;

    Net();

public:
    // Disable the copy constructor and the copy assignment operator
    Net(const Net&) = delete;
    Net& operator=(const Net&) = delete;
    ~Net();

    static Net& get();

    /*
     * Net::serialize()
     *
     * It serializes the packet and stores it in the buffer with the returned
     * value being the length.
     *
     * NOTE: the buffer should freed later by Net::free().
     */
    uint32_t serialize(uint8_t **buffer, const Packet& pkt,
                       const uint8_t *src_mac,
                       const uint8_t *dst_mac) const;
    void free(uint8_t *) const;
};
