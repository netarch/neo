#include "middlebox.hpp"

#include <cassert>
#include <cmath>

#include "droptimeout.hpp"
#include "emulationmgr.hpp"
#include "stats.hpp"

using namespace std;

Middlebox::Middlebox() : _emulation(nullptr) {}

int Middlebox::rewind(NodePacketHistory *nph) {
    _emulation = EmulationMgr::get().get_emulation(this, nph);
    return _emulation->rewind(nph);
}

void Middlebox::set_node_pkt_hist(NodePacketHistory *nph) {
    assert(_emulation->mb() == this);
    EmulationMgr::get().update_node_pkt_hist(_emulation, nph);
}

std::list<Packet> Middlebox::send_pkt(const Packet &pkt) {
    assert(_emulation->mb() == this);
    std::list<Packet> recv_pkts = _emulation->send_pkt(pkt);
    DropTimeout::get().update_timeout(recv_pkts.size());
    return recv_pkts;
}

std::set<FIB_IPNH>
Middlebox::get_ipnhs(const IPv4Address &dst __attribute__((unused)),
                     const RoutingTable *rib __attribute__((unused)),
                     std::unordered_set<IPv4Address> *looked_up_ips
                     __attribute__((unused))) {
    return std::set<FIB_IPNH>();
}
