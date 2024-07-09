#pragma once

#include <linux/if_ether.h>

#include "interface.hpp"

class PktBuffer {
private:
    Interface *intf;
    uint8_t buffer[ETH_FRAME_LEN];
    size_t len;

public:
    PktBuffer(Interface *);
    PktBuffer(const PktBuffer &) = default;
    PktBuffer(PktBuffer &&)      = default;

    PktBuffer &operator=(const PktBuffer &) = default;
    PktBuffer &operator=(PktBuffer &&)      = default;

    Interface *get_intf() const;
    uint8_t *get_buffer();
    const uint8_t *get_buffer() const;
    size_t get_len() const;

    void set_intf(Interface *);
    void set_len(size_t);
};
