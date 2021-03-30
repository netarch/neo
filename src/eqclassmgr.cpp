#include "eqclassmgr.hpp"

#include <cassert>
#include <iterator>

#include "network.hpp"
#include "middlebox.hpp"
#include "process/openflow.hpp"
#include "lib/logger.hpp"

EqClassMgr::~EqClassMgr()
{
    for (EqClass * const& ec : all_ECs) {
        delete ec;
    }
}

EqClassMgr& EqClassMgr::get()
{
    static EqClassMgr instance;
    return instance;
}

void EqClassMgr::split_intersected_ec(EqClass *ec, const ECRange& range, bool owned)
{
    EqClass *new_ec = new EqClass();
    const EqClass orig_ec(*ec);

    for (ECRange ecrange : orig_ec) {
        if (ecrange == range) {
            // overlapped range, split at the intersections (if any)
            allranges.erase(ecrange);
            ec->rm_range(ecrange);
            if (ecrange.get_lb() < range.get_lb()) {
                ECRange lower_new_range(ecrange);
                lower_new_range.set_ub(IPv4Address(range.get_lb() - 1));
                ec->add_range(lower_new_range);
                allranges.insert(lower_new_range);
                ecrange.set_lb(range.get_lb());
            }
            if (range.get_ub() < ecrange.get_ub()) {
                ECRange upper_new_range(ecrange);
                upper_new_range.set_lb(IPv4Address(range.get_ub() + 1));
                ec->add_range(upper_new_range);
                allranges.insert(upper_new_range);
                ecrange.set_ub(range.get_ub());
            }
            // move the contained range to the new EC
            ecrange.set_ec(new_ec);
            new_ec->add_range(ecrange);
            allranges.insert(ecrange);
        } // else: non-overlapped range, do nothing (stay in the original EC)
    }

    assert(!new_ec->empty());
    all_ECs.insert(new_ec);
    if (owned || owned_ECs.count(ec) > 0) {
        owned_ECs.insert(new_ec);
    }
}

void EqClassMgr::add_non_overlapped_ec(const ECRange& range, bool owned)
{
    EqClass *new_ec = new EqClass();
    IPv4Address lb = range.get_lb();
    std::set<ECRange>::const_iterator it;

    for (it = allranges.begin(); it != allranges.end() && *it != range; ++it);
    for (; it != allranges.end() && *it == range; ++it) {
        if (lb < it->get_lb()) {
            ECRange new_range(lb, IPv4Address(it->get_lb() - 1));
            new_range.set_ec(new_ec);
            new_ec->add_range(new_range);
            allranges.insert(new_range);
        }
        lb = it->get_ub() + 1;
    }
    if (lb <= range.get_ub()) {
        ECRange new_range(lb, range.get_ub());
        new_range.set_ec(new_ec);
        new_ec->add_range(new_range);
        allranges.insert(new_range);
    }

    if (new_ec->empty()) {
        delete new_ec;
    } else {
        all_ECs.insert(new_ec);
        if (owned) {
            owned_ECs.insert(new_ec);
        }
    }
}

void EqClassMgr::add_ec(const ECRange& new_range, bool owned)
{
    std::set<EqClass *> overlapped_ecs = get_overlapped_ecs(new_range);

    // add overlapped ECs
    for (EqClass *ec : overlapped_ecs) {
        if (!new_range.contains(*ec)) {
            // ec is partially inside the new range
            split_intersected_ec(ec, new_range, owned);
        } else if (owned) {
            owned_ECs.insert(ec);
        }
    }

    // add non-overlapped subranges of the new range, if any, to a new EC
    add_non_overlapped_ec(new_range, owned);
}

void EqClassMgr::add_ec(const IPNetwork<IPv4Address>& net)
{
    add_ec(ECRange(net), false);
}

void EqClassMgr::add_ec(const IPv4Address& addr, bool owned)
{
    add_ec(ECRange(addr, addr), owned);
}

void EqClassMgr::compute_policy_oblivious_ecs(
    const Network& network, const OpenflowProcess& openflow)
{
    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            this->add_ec(intf.first, /* owned */ true);
        }
        for (const Route& route : node.second->get_rib()) {
            this->add_ec(route.get_network());
        }
    }

    for (const auto& update : openflow.get_updates()) {
        for (const Route& update_route : update.second) {
            this->add_ec(update_route.get_network());
        }
    }

    for (const Middlebox *mb : network.get_middleboxes()) {
        MB_App *app = mb->get_app();
        for (const auto& prefix : app->get_ip_prefixes()) {
            this->add_ec(prefix);
        }
        for (const auto& addr : app->get_ip_addrs()) {
            this->add_ec(addr);
        }
        const auto& app_ports = app->get_ports();
        this->ports.insert(app_ports.begin(), app_ports.end());
    }
}

std::set<EqClass *> EqClassMgr::get_overlapped_ecs(
    const ECRange& range, bool owned_only) const
{
    std::set<EqClass *> overlapped_ecs;
    auto ecrange = allranges.find(range);
    if (ecrange != allranges.end()) {
        for (std::set<ECRange>::const_reverse_iterator r
                = std::make_reverse_iterator(ecrange);
                r != allranges.rend() && *r == range; ++r) {
            if (!owned_only || owned_ECs.count(r->get_ec()) > 0) {
                overlapped_ecs.insert(r->get_ec());
            }
        }
        for (std::set<ECRange>::const_iterator r = ecrange;
                r != allranges.end() && *r == range; ++r) {
            if (!owned_only || owned_ECs.count(r->get_ec()) > 0) {
                overlapped_ecs.insert(r->get_ec());
            }
        }
    }
    return overlapped_ecs;
}

const std::set<uint16_t>& EqClassMgr::get_ports() const
{
    return ports;
}

EqClass *EqClassMgr::find_ec(const IPv4Address& ip) const
{
    auto it = allranges.find(ECRange(ip, ip));
    if (it == allranges.end()) {
        Logger::error("Cannot find the EC of " + ip.to_string());
        return nullptr;
    }
    return it->get_ec();
}
