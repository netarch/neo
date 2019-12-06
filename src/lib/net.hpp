#pragma once

#include <libnet.h>

#include "packet.hpp"

class Net
{
private:
    libnet_t *l;
    char errbuf[LIBNET_ERRBUF_SIZE];

    Net();

public:
    // Disable the copy constructor and the copy assignment operator
    Net(const Net&) = delete;
    void operator=(const Net&) = delete;
    ~Net();

    static Net& get();

    /*
     * Net::get_pkt()
     *
     * It concretizes the abstract Packet "pkt" and stores the generated
     * concrete packet in "packet" with the returned value being the length.
     *
     * NOTE: the concrete packet should freed later by Net::free_pkt().
     */
    uint32_t get_pkt(uint8_t **packet,
                     const Packet& pkt,
                     // L7
                     const uint8_t *payload,
                     uint32_t payload_size,
                     // TCP
                     uint16_t src_port,
                     uint16_t dst_port,
                     uint32_t seq,
                     uint32_t ack,
                     uint8_t ctrl_flags,
                     // Ethernet
                     const uint8_t *src_mac,
                     const uint8_t *dst_mac);
    void free_pkt(uint8_t *);
};
