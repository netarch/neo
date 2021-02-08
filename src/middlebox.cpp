#include "middlebox.hpp"

#include <cassert>

#include "emulationmgr.hpp"

Middlebox::Middlebox()
    : emulation(nullptr), app(nullptr)
{
}

Middlebox::~Middlebox()
{
    delete app;
}

std::string Middlebox::get_env() const
{
    return env;
}

MB_App *Middlebox::get_app() const
{
    return app;
}

std::chrono::microseconds Middlebox::get_timeout() const
{
    return timeout;
}

int Middlebox::rewind(NodePacketHistory *nph)
{
    emulation = EmulationMgr::get().get_emulation(this, nph);
    return emulation->rewind(nph);
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph)
{
    assert(emulation->get_mb() == this);
    EmulationMgr::get().update_node_pkt_hist(emulation, nph);
}

std::vector<Packet> Middlebox::send_pkt(const Packet& pkt)
{
    assert(emulation->get_mb() == this);
    return emulation->send_pkt(pkt);
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)),
    const RoutingTable *rib __attribute__((unused)),
    std::unordered_set<IPv4Address> *looked_up_ips __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}
