#include "pktbuffer.hpp"

PktBuffer::PktBuffer(Interface *intf): intf(intf), len(ETH_FRAME_LEN)
{
}

Interface *PktBuffer::get_intf() const
{
    return intf;
}

uint8_t *PktBuffer::get_buffer()
{
    return buffer;
}

const uint8_t *PktBuffer::get_buffer() const
{
    return buffer;
}

size_t PktBuffer::get_len() const
{
    return len;
}

void PktBuffer::set_intf(Interface *i)
{
    intf = i;
}

void PktBuffer::set_len(size_t l)
{
    len = l;
}
