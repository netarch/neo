#include "eqclasses.hpp"

#include <algorithm>

#include "lib/logger.hpp"

EqClasses::EqClasses(EqClass *ec)
{
    if (ec) {
        for (const ECRange& range : *ec) {
            allranges.insert(range);
        }
    }
    ECs.insert(ec);
}

EqClasses::~EqClasses()
{
    for (EqClass *const ec : ECs) {
        delete ec;
    }
}

std::set<EqClass *> EqClasses::get_overlapped_ecs(const ECRange& range) const
{
    std::set<EqClass *> overlapped_ecs;
    auto ecrange = allranges.find(range);
    if (ecrange != allranges.end()) {
        for (std::set<ECRange>::const_reverse_iterator r
                = std::make_reverse_iterator(ecrange);
                r != allranges.rend() && *r == range; ++r) {
            overlapped_ecs.insert(r->get_ec());
        }
        for (std::set<ECRange>::const_iterator r = ecrange;
                r != allranges.end() && *r == range; ++r) {
            overlapped_ecs.insert(r->get_ec());
        }
    }
    return overlapped_ecs;
}

void EqClasses::split_intersected_ec(EqClass *ec, const ECRange& range)
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

    ECs.insert(new_ec);
}

void EqClasses::add_non_overlapped_ec(const ECRange& range)
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
        ECs.insert(new_ec);
    }
}

void EqClasses::add_ec(const ECRange& new_range)
{
    std::set<EqClass *> overlapped_ecs = get_overlapped_ecs(new_range);

    // add overlapped ECs
    for (EqClass *ec : overlapped_ecs) {
        if (!new_range.contains(*ec)) {
            // ec is partially inside the new range
            split_intersected_ec(ec, new_range);
        }
    }

    // add non-overlapped subranges of the new range, if any, to a new EC
    add_non_overlapped_ec(new_range);
}

void EqClasses::add_ec(const IPNetwork<IPv4Address>& net)
{
    add_ec(ECRange(net));
}

void EqClasses::add_ec(const IPv4Address& addr)
{
    add_ec(ECRange(addr, addr));
}

void EqClasses::add_mask_range(const ECRange& mask_range,
                               const EqClasses& all_ECs)
{
    clear();
    std::set<EqClass *> overlapped_ecs = all_ECs.get_overlapped_ecs(mask_range);

    for (const EqClass *ec : overlapped_ecs) {
        EqClass *new_ec = new EqClass();
        for (ECRange ecrange : *ec) {
            if (ecrange == mask_range) {
                // overlapped range, trim at the intersections (if any)
                if (ecrange.get_lb() < mask_range.get_lb()) {
                    ecrange.set_lb(mask_range.get_lb());
                }
                if (mask_range.get_ub() < ecrange.get_ub()) {
                    ecrange.set_ub(mask_range.get_ub());
                }
                // add the contained range to the new EC
                ecrange.set_ec(new_ec);
                new_ec->add_range(ecrange);
                allranges.insert(ecrange);
            } // else: non-overlapped range, do nothing
        }
        ECs.insert(new_ec);
    }
}

std::string EqClasses::to_string() const
{
    std::string ret;
    for (EqClass *const ec : ECs) {
        ret += ec->to_string() + "\n";
    }
    return ret;
}

bool EqClasses::empty() const
{
    return ECs.empty();
}

EqClasses::size_type EqClasses::size() const
{
    return ECs.size();
}

EqClass *EqClasses::find_ec(const IPv4Address& ip) const
{
    auto it = allranges.find(ECRange(ip, ip));
    if (it == allranges.end()) {
        return nullptr;
    }
    return it->get_ec();
}

EqClasses::iterator EqClasses::erase(const_iterator pos)
{
    return ECs.erase(pos);
}

void EqClasses::clear()
{
    for (EqClass *const ec : ECs) {
        delete ec;
    }
    allranges.clear();
    ECs.clear();
}

EqClasses::iterator EqClasses::begin()
{
    return ECs.begin();
}

EqClasses::const_iterator EqClasses::begin() const
{
    return ECs.begin();
}

EqClasses::iterator EqClasses::end()
{
    return ECs.end();
}

EqClasses::const_iterator EqClasses::end() const
{
    return ECs.end();
}

EqClasses::reverse_iterator EqClasses::rbegin()
{
    return ECs.rbegin();
}

EqClasses::const_reverse_iterator EqClasses::rbegin() const
{
    return ECs.rbegin();
}

EqClasses::reverse_iterator EqClasses::rend()
{
    return ECs.rend();
}

EqClasses::const_reverse_iterator EqClasses::rend() const
{
    return ECs.rend();
}
