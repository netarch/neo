#pragma once

#include <list>

#include "packet.hpp"

class Middlebox;

class Driver {
public:
    virtual ~Driver() = default;

    virtual void init() = 0;
    virtual void reset() = 0;
    virtual void pause() = 0;
    virtual void unpause() = 0;
    virtual size_t inject_packet(const Packet &) = 0;
    virtual std::list<Packet> read_packets() const = 0;
};
