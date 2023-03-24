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
    std::list<Packet> recv_pkts = _emulation->send_pkt(pkt);
    DropTimeout::get().update_timeout(recv_pkts.size());
    return recv_pkts;
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
std::set<FIB_IPNH>
Middlebox::get_ipnhs(const IPv4Address &dst __attribute__((unused)),
                     const RoutingTable *rib __attribute__((unused)),
                     std::unordered_set<IPv4Address> *looked_up_ips
                     __attribute__((unused))) {
    return std::set<FIB_IPNH>();
}
