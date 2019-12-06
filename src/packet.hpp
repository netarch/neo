#pragma once

#include "lib/ip.hpp"

class Packet
{
private:
    ;

    friend struct PacketHash;
    friend bool operator==(const Packet&, const Packet&);

public:
    //Packet(something, const IPv4Address& dst_ip);
};

bool operator==(const Packet&, const Packet&);

struct PacketHash {
    size_t operator()(Packet *const& p) const
    {
        return (size_t)p;   // TODO
    }
};
