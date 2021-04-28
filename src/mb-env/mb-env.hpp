#pragma once

#include <vector>
#include <string>

#include "node.hpp"
#include "packet.hpp"
#include "mb-app/mb-app.hpp"

class MB_Env
{
public:
    virtual ~MB_Env() = default;

    virtual void init(const Node&) = 0;
    virtual void run(void (*)(MB_App *), MB_App *) = 0;
    virtual size_t inject_packet(const Packet&) = 0;
    virtual std::vector<Packet> read_packets() const = 0;
    virtual std::string packet_drop() = 0;
};
