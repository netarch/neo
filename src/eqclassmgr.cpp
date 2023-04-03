#include "eqclassmgr.hpp"

#include <cassert>
#include <iterator>
#include <random>
#include <string>

#include "logger.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "process/openflow.hpp"

using namespace std;

EqClassMgr::~EqClassMgr() {
    reset();
}

EqClassMgr &EqClassMgr::get() {
    static EqClassMgr instance;
    return instance;
}

void EqClassMgr::reset() {
    for (EqClass *const &ec : _all_ecs) {
        delete ec;
    }

    _allranges.clear();
    _all_ecs.clear();
    _owned_ecs.clear();
    _ports.clear();
}

void EqClassMgr::split_intersected_ec(EqClass *ec,
                                      const ECRange &range,
                                      bool owned) {
    EqClass *new_ec = new EqClass();
    const EqClass orig_ec(*ec);

    for (ECRange ecrange : orig_ec) {
        if (ecrange == range) {
            // overlapped range, split at the intersections (if any)
            _allranges.erase(ecrange);
            ec->rm_range(ecrange);
            if (ecrange.get_lb() < range.get_lb()) {
                ECRange lower_new_range(ecrange);
                lower_new_range.set_ub(IPv4Address(range.get_lb() - 1));
                ec->add_range(lower_new_range);
                _allranges.insert(lower_new_range);
                ecrange.set_lb(range.get_lb());
            }
            if (range.get_ub() < ecrange.get_ub()) {
                ECRange upper_new_range(ecrange);
                upper_new_range.set_lb(IPv4Address(range.get_ub() + 1));
                ec->add_range(upper_new_range);
                _allranges.insert(upper_new_range);
                ecrange.set_ub(range.get_ub());
            }
            // move the contained range to the new EC
            ecrange.set_ec(new_ec);
            new_ec->add_range(ecrange);
            _allranges.insert(ecrange);
        } // else: non-overlapped range, do nothing (stay in the original EC)
    }

    assert(!new_ec->empty());
    _all_ecs.insert(new_ec);
    if (owned || _owned_ecs.count(ec) > 0) {
        _owned_ecs.insert(new_ec);
    }
}

void EqClassMgr::add_non_overlapped_ec(const ECRange &range, bool owned) {
    EqClass *new_ec = new EqClass();
    IPv4Address lb = range.get_lb();
    set<ECRange>::const_iterator it;

    for (it = _allranges.begin(); it != _allranges.end() && *it != range; ++it)
        ;
    for (; it != _allranges.end() && *it == range; ++it) {
        if (lb < it->get_lb()) {
            ECRange new_range(lb, IPv4Address(it->get_lb() - 1));
            new_range.set_ec(new_ec);
            new_ec->add_range(new_range);
            _allranges.insert(new_range);
        }
        lb = it->get_ub() + 1;
    }
    if (lb <= range.get_ub()) {
        ECRange new_range(lb, range.get_ub());
        new_range.set_ec(new_ec);
        new_ec->add_range(new_range);
        _allranges.insert(new_range);
    }

    if (new_ec->empty()) {
        delete new_ec;
    } else {
        _all_ecs.insert(new_ec);
        if (owned) {
            _owned_ecs.insert(new_ec);
        }
    }
}

void EqClassMgr::add_ec(const ECRange &new_range, bool owned) {
    set<EqClass *> overlapped_ecs = get_overlapped_ecs(new_range);

    // add overlapped ECs
    for (EqClass *ec : overlapped_ecs) {
        if (!new_range.contains(*ec)) {
            // ec is partially inside the new range
            split_intersected_ec(ec, new_range, owned);
        } else if (owned) {
            _owned_ecs.insert(ec);
        }
    }

    // add non-overlapped subranges of the new range, if any, to a new EC
    add_non_overlapped_ec(new_range, owned);
}

void EqClassMgr::add_ec(const IPNetwork<IPv4Address> &net) {
    add_ec(ECRange(net), false);
}

void EqClassMgr::add_ec(const IPv4Address &addr, bool owned) {
    add_ec(ECRange(addr, addr), owned);
}

void EqClassMgr::compute_initial_ecs(const Network &network,
                                     const OpenflowProcess &openflow) {
    for (const auto &node : network.nodes()) {
        for (const auto &[addr, intf] : node.second->get_intfs_l3()) {
            this->add_ec(addr, /* owned */ true);
        }
        for (const Route &route : node.second->get_rib()) {
            this->add_ec(route.get_network());
        }
    }

    for (const auto &update : openflow.get_updates()) {
        for (const Route &update_route : update.second) {
            this->add_ec(update_route.get_network());
        }
    }

    for (const Middlebox *mb : network.middleboxes()) {
        for (const auto &prefix : mb->ec_ip_prefixes()) {
            this->add_ec(prefix);
        }
        for (const auto &addr : mb->ec_ip_addrs()) {
            this->add_ec(addr);
        }
        const auto &app_ports = mb->ec_ports();
        this->_ports.insert(app_ports.begin(), app_ports.end());
    }

    // Add another random port denoting the "other" port EC
    uint16_t port;
    default_random_engine generator;
    uniform_int_distribution<uint16_t> distribution(10, 49151);
    do {
        port = distribution(generator);
    } while (this->_ports.count(port) > 0);
    this->_ports.insert(port);
}

set<EqClass *> EqClassMgr::get_overlapped_ecs(const ECRange &range,
                                              bool owned_only) const {
    set<EqClass *> overlapped_ecs;
    auto ecrange = _allranges.find(range);
    if (ecrange != _allranges.end()) {
        for (set<ECRange>::const_reverse_iterator r =
                 make_reverse_iterator(ecrange);
             r != _allranges.rend() && *r == range; ++r) {
            if (!owned_only || _owned_ecs.count(r->get_ec()) > 0) {
                overlapped_ecs.insert(r->get_ec());
            }
        }
        for (set<ECRange>::const_iterator r = ecrange;
             r != _allranges.end() && *r == range; ++r) {
            if (!owned_only || _owned_ecs.count(r->get_ec()) > 0) {
                overlapped_ecs.insert(r->get_ec());
            }
        }
    }
    return overlapped_ecs;
}

EqClass *EqClassMgr::find_ec(const IPv4Address &ip) const {
    auto it = _allranges.find(ECRange(ip, ip));
    if (it == _allranges.end()) {
        logger.error("Cannot find the EC of " + ip.to_string());
        return nullptr;
    }
    return it->get_ec();
}
