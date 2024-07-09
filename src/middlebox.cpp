#include "middlebox.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <unordered_set>

#include "emulationmgr.hpp"
#include "injection-result.hpp"
#include "stats.hpp"
#include "unique-storage.hpp"

using namespace std;

void Middlebox::rewind(NodePacketHistory *nph) {
    _STATS_START(Stats::Op::LAUNCH_OR_GET_EMU);
    _emulation = EmulationMgr::get().get_emulation(this, nph);
    _STATS_STOP(Stats::Op::LAUNCH_OR_GET_EMU);
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
 * Sends a packet to the emulation.
 *
 * @param pkt the packet to be sent
 *
 * @return InjectionResults - A sorted vector of unique injection results.
 */
InjectionResults Middlebox::send_pkt(const Packet &pkt) {
    assert(_emulation->mb() == this);
    assert(_packets_per_injection > 0);
    logger.info("Sending the packet " + std::to_string(_packets_per_injection) +
                " times");

    // Since we expect the number of duplicate injection results will be small,
    // using a vector should be more performant.
    InjectionResults results;
    for (int i = 0; i < _packets_per_injection; ++i) {
        InjectionResult *ir = new InjectionResult(_emulation->send_pkt(pkt));
        ir                  = storage.store_injection_result(ir);
        results.add(ir);
    }

    logger.info("Got " + std::to_string(results.size()) +
                " distinct injection results");
    return results;
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
