#pragma once

#include <list>

#include "mb-app/mb-app.hpp"
#include "packet.hpp"

class Middlebox;

class MB_Env {
public:
    virtual ~MB_Env() = default;

    virtual void init(const Middlebox &) = 0;
    virtual void run(void (*)(MB_App *), MB_App *) = 0;
    virtual size_t inject_packet(const Packet &) = 0;
    virtual std::list<Packet> read_packets() const = 0;
};
