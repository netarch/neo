#include "eqclasses.hpp"
#include "lib/logger.hpp"

std::shared_ptr<EqClass> EqClasses::split_intersected_ec(
    std::shared_ptr<EqClass> ec, const ECRange& range)
{
    std::shared_ptr<EqClass> new_ec = std::make_shared<EqClass>();

    for (ECRange ecrange : ec->get_ranges()) {
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

std::shared_ptr<EqClass> EqClasses::add_non_overlapped_ec(const ECRange& range)
{
    std::shared_ptr<EqClass> new_ec = nullptr;
    IPv4Address lb = range.get_lb();
    std::set<ECRange>::const_iterator it;

    for (it = allranges.begin(); it != allranges.end() && *it != range; ++it);
    for (; it != allranges.end() && *it == range; ++it) {
        if (lb < it->get_lb()) {
            ECRange new_range(lb, IPv4Address(it->get_lb() - 1));
            if (!new_ec) {
                new_ec = std::make_shared<EqClass>();
            }
            new_range.set_ec(new_ec);
            new_ec->add_range(new_range);
            allranges.insert(new_range);
        }
        lb = it->get_ub() + 1;
    }
    if (lb <= range.get_ub()) {
        ECRange new_range(lb, range.get_ub());
        if (!new_ec) {
            new_ec = std::make_shared<EqClass>();
        }
        new_range.set_ec(new_ec);
        new_ec->add_range(new_range);
        allranges.insert(new_range);
    }
    if (new_ec) {
        ECs.insert(new_ec);
    }
    return new_ec;
}

std::set<std::shared_ptr<EqClass> > EqClasses::add_ec(const ECRange& range)
{
    std::set<std::shared_ptr<EqClass> > new_ecs;

    // get overlapped ECs
    std::set<std::shared_ptr<EqClass> > overlapped_ecs;
    if (allranges.find(range) != allranges.end()) {
        for (const ECRange& ecrange : allranges) {
            if (ecrange == range) {
                overlapped_ecs.insert(ecrange.get_ec());
            }
        }
    }

    // add overlapped ECs
    for (const std::shared_ptr<EqClass>& ec : overlapped_ecs) {
        new_ecs.insert(ec);
        if (!range.contains(*ec)) {
            // EC is partially inside the new range
            new_ecs.insert(split_intersected_ec(ec, range));
        }
    }

    // add non-overlapped subranges of the new range, if any, to a new EC
    std::shared_ptr<EqClass> new_ec = add_non_overlapped_ec(range);
    if (new_ec) {
        new_ecs.insert(new_ec);
    }

    return new_ecs;
}

std::set<std::shared_ptr<EqClass> > EqClasses::add_ec(
    const IPNetwork<IPv4Address>& net)
{
    return add_ec(ECRange(net));
}

std::string EqClasses::to_string() const
{
    std::string ret;
    for (const std::shared_ptr<EqClass>& ec : ECs) {
        ret += ec->to_string() + "\n";
    }
    return ret;
}

EqClasses::size_type EqClasses::size() const
{
    return ECs.size();
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
