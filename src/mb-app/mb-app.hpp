#pragma once

#include <set>

#include "lib/ip.hpp"

class MB_App
{
protected:
    // interesting IP and port values parsed from the appliance configs
    std::set<IPNetwork<IPv4Address>> ip_prefixes;
    std::set<IPv4Address> ip_addrs;
    std::set<uint16_t> ports;

protected:
    friend class Config;
    MB_App() = default;

public:
    virtual ~MB_App() = default;

    const std::set<IPNetwork<IPv4Address>>& get_ip_prefixes() const;
    const std::set<IPv4Address>& get_ip_addrs() const;
    const std::set<uint16_t>& get_ports() const;

    /* Note that all internal states should be flushed/reset */
    virtual void init() = 0;    // hard-reset, restart, start
    virtual void reset() = 0;   // soft-reset, reload
};

void mb_app_init(MB_App *);
void mb_app_reset(MB_App *);
