#pragma once

#include <set>
#include <memory>
#include <string>

class EqClass;
#include "ecrange.hpp"

/*
 * An EqClass (instance) is a set of ECRanges (a continuous range of IP
 * addresses), where any packet with the destination inside the ranges has the
 * same behavior within the current network configuration.
 */
class EqClass
{
private:
    std::set<ECRange> ranges;

public:
    EqClass() = default;

    std::string to_string() const;
    bool empty() const;
    const std::set<ECRange>& get_ranges() const;
    void add_range(const ECRange&);
    void rm_range(const ECRange&);
};
