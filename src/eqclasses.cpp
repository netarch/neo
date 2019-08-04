#include <algorithm>

#include "eqclasses.hpp"
#include "lib/logger.hpp"

EqClasses::~EqClasses()
{
    for (const EqClass *ec : ECs) {
        delete ec;
    }
}

EqClass *EqClasses::split_intersected_ec( EqClass *ec, const ECRange& range)
{
    EqClass *new_ec = new EqClass();

    for (ECRange ecrange : *ec) {
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
    return new_ec;
}

EqClass *EqClasses::add_non_overlapped_ec(const ECRange& range)
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
        return nullptr;
    } else {
        ECs.insert(new_ec);
        return new_ec;
    }
}

std::set<EqClass *> EqClasses::add_ec(const ECRange& range)
{
    std::set<EqClass *> new_ecs;

    // apply mask range
    IPv4Address lb = std::max(range.get_lb(), mask_range.get_lb());
    IPv4Address ub = std::min(range.get_ub(), mask_range.get_ub());
    if (lb > ub) {
        return new_ecs;
    }
    ECRange new_range(lb, ub);

    // get overlapped ECs
    std::set<EqClass *> overlapped_ecs;
    if (allranges.find(new_range) != allranges.end()) {
        for (const ECRange& ecrange : allranges) {
            if (ecrange == new_range) {
                overlapped_ecs.insert(ecrange.get_ec());
            }
        }
    }

    // add overlapped ECs
    for (EqClass *ec : overlapped_ecs) {
        new_ecs.insert(ec);
        if (!new_range.contains(*ec)) {
            // EC is partially inside the new range
            new_ecs.insert(split_intersected_ec(ec, new_range));
        }
    }

    // add non-overlapped subranges of the new range, if any, to a new EC
    EqClass *new_ec = add_non_overlapped_ec(new_range);
    if (new_ec) {
        new_ecs.insert(new_ec);
    }

    return new_ecs;
}

std::set<EqClass *> EqClasses::add_ec(const IPNetwork<IPv4Address>& net)
{
    return add_ec(ECRange(net));
}

std::set<EqClass *> EqClasses::add_ec(const IPv4Address& addr)
{
    return add_ec(ECRange(addr, addr));
}

void EqClasses::set_mask_range(const IPRange<IPv4Address>& range)
{
    mask_range = range;
}

void EqClasses::set_mask_range(IPRange<IPv4Address>&& range)
{
    mask_range = range;
}

std::string EqClasses::to_string() const
{
    std::string ret;
    for (const EqClass *ec : ECs) {
        ret += ec->to_string() + "\n";
    }
    return ret;
}

EqClasses::size_type EqClasses::size() const
{
    return ECs.size();
}

EqClasses::iterator EqClasses::erase(const_iterator pos)
{
    return ECs.erase(pos);
}

void EqClasses::clear()
{
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
