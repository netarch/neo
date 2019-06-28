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
    typedef std::set<ECRange>::iterator iterator;
    typedef std::set<ECRange>::const_iterator const_iterator;
    typedef std::set<ECRange>::reverse_iterator reverse_iterator;
    typedef std::set<ECRange>::const_reverse_iterator const_reverse_iterator;

    EqClass() = default;

    std::string to_string() const;
    bool empty() const;
    void add_range(const ECRange&);
    void rm_range(const ECRange&);

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};
