#pragma once

#include <cstdint>
#include <string>

struct libnet_context;
typedef struct libnet_context libnet_t;
class Packet;
class PktBuffer;

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
     * It serializes the packet and stores it in buffer.
     * NOTE: the buffer should be freed later by calling Net::free().
     */
    void serialize(uint8_t **buffer, uint32_t *buffer_size, const Packet& pkt,
                   const uint8_t *src_mac, const uint8_t *dst_mac) const;
    void free(uint8_t *) const;

    /*
     * Net::deserialize()
     * It deserializes the buffer into packet. If the buffer is ill-formed, the
     * packet would be empty.
     * NOTE: if the packet is a TCP packet, it should later be passed to
     * convert_tcp_flags to fully deserialize the packet content.
     */
    void deserialize(Packet&, const PktBuffer&) const;
    void convert_tcp_flags(Packet&, uint8_t old_pkt_state,
                           bool change_direction) const;

    std::string mac_to_str(const uint8_t *) const;
};
