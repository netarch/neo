#pragma once

#include "node.hpp"
#include "interface.hpp"
#include "mb-app/mb-app.hpp"

class MB_Env
{
public:
    virtual ~MB_Env() = default;

    virtual void init(const Node *) = 0;
    virtual void run(void (*)(MB_App *), MB_App *) = 0;
    virtual void get_mac(Interface *, uint8_t *) const = 0;
    virtual size_t inject_packet(Interface *, const uint8_t *buf, size_t len) = 0;
};
