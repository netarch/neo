#pragma once

#include <set>
#include <memory>
#include <string>

#include "eqclass.hpp"

class EqClasses
{
private:
    std::set<ECRange> allranges;
    std::set<std::shared_ptr<EqClass> > ECs;
    IPRange<IPv4Address> mask_range;

    std::shared_ptr<EqClass> split_intersected_ec(std::shared_ptr<EqClass> ec,
            const ECRange& range);
    std::shared_ptr<EqClass> add_non_overlapped_ec(const ECRange&);

public:
    typedef std::set<std::shared_ptr<EqClass> >::size_type size_type;
    typedef std::set<std::shared_ptr<EqClass> >::iterator iterator;
    typedef std::set<std::shared_ptr<EqClass> >::const_iterator const_iterator;
    typedef std::set<std::shared_ptr<EqClass> >::reverse_iterator
    reverse_iterator;
    typedef std::set<std::shared_ptr<EqClass> >::const_reverse_iterator
    const_reverse_iterator;

    EqClasses() = default;

    std::set<std::shared_ptr<EqClass> > add_ec(const ECRange&);
    std::set<std::shared_ptr<EqClass> > add_ec(const IPNetwork<IPv4Address>&);
    std::set<std::shared_ptr<EqClass> > add_ec(const IPv4Address&);
    void set_mask_range(const IPRange<IPv4Address>&);
    void set_mask_range(IPRange<IPv4Address>&&);

    std::string to_string() const;
    size_type size() const;
    iterator erase(const_iterator);
    void clear();

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};
