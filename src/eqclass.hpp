#pragma once

#include <set>
#include <string>

class EqClass;
#include "ecrange.hpp"

/*
 * An EqClass (instance) is a set of ECRanges (a continuous range of IP
 * addresses), where any packet with its destination (or source) inside the
 * ranges has the same behavior. The ranges are disjoint.
 */
class EqClass
{
private:
    std::set<ECRange> ranges;

    friend bool operator==(const EqClass&, const EqClass&);

public:
    typedef std::set<ECRange>::iterator iterator;
    typedef std::set<ECRange>::const_iterator const_iterator;
    typedef std::set<ECRange>::reverse_iterator reverse_iterator;
    typedef std::set<ECRange>::const_reverse_iterator const_reverse_iterator;

    std::string to_string() const;
    bool empty() const;
    void add_range(const ECRange&);
    void rm_range(const ECRange&);
    bool contains(const IPv4Address&) const;
    IPv4Address representative_addr() const;

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};

bool operator==(const EqClass&, const EqClass&);
