#include "mb-app/mb-app.hpp"

const std::set<IPNetwork<IPv4Address>>& MB_App::get_ip_prefixes() const
{
    return this->ip_prefixes;
}

const std::set<IPv4Address>& MB_App::get_ip_addrs() const
{
    return this->ip_addrs;
}

const std::set<uint16_t>& MB_App::get_ports() const
{
    return this->ports;
}

void mb_app_init(MB_App *app)
{
    app->init();
}

void mb_app_reset(MB_App *app)
{
    app->reset();
}
