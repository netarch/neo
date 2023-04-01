#include "middlebox.hpp"

#include <cassert>
#include <cmath>

#include "emulationmgr.hpp"
#include "stats.hpp"

using namespace std;

Middlebox::Middlebox() : _emulation(nullptr) {}

void Middlebox::rewind(NodePacketHistory *nph) {
    _emulation = EmulationMgr::get().get_emulation(this, nph);
    _STATS_START(Stats::Op::REWIND);
    int rewind_injections = _emulation->rewind(nph);
    _STATS_STOP(Stats::Op::REWIND);
    _STATS_REWIND_INJECTION(rewind_injections);
}

/**
 * It updates the packet history of a node in the emulation
 *
 * @param nph The NodePacketHistory object that you want to set for this
 * middlebox.
 */
void Middlebox::set_node_pkt_hist(NodePacketHistory *nph) {
    assert(_emulation->mb() == this);
    EmulationMgr::get().update_node_pkt_hist(_emulation, nph);
}

/**
 * It sends a packet to the emulation, and updates the drop timeout.
 *
 * @param pkt the packet to send
 *
 * @return A list of received packets.
 */
std::list<Packet> Middlebox::send_pkt(const Packet &pkt) {
    assert(_emulation->mb() == this);
    return _emulation->send_pkt(pkt);
}

/**
 * It returns an empty set. We use emulations to get next hops for middleboxes
 * by injecting concrete packets to emulation instances.
 *
 * @param dst the destination IP address
 * @param rib the RoutingTable to use for recursive lookups. If nullptr, use
 * this node's RoutingTable.
 * @param looked_up_ips a set of IP addresses that have already been looked up.
 *
 * @return A set of FIB_IPNHs
 */
std::set<FIB_IPNH> Middlebox::get_ipnhs(
    [[maybe_unused]] const IPv4Address &dst,
    [[maybe_unused]] const RoutingTable *rib,
    [[maybe_unused]] std::unordered_set<IPv4Address> *looked_up_ips) {
    return std::set<FIB_IPNH>();
}
