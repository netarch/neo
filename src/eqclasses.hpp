#pragma once

#include <set>
#include <memory>
#include <string>

#include "eqclass.hpp"

class EqClasses
{
private:
    std::set<ECRange> allranges;
    std::set<EqClass *> ECs;

    std::set<EqClass *> get_overlapped_ecs(const ECRange&) const;
    void split_intersected_ec(EqClass *ec, const ECRange& range);
    void add_non_overlapped_ec(const ECRange&);

public:
    typedef std::set<EqClass *>::size_type size_type;
    typedef std::set<EqClass *>::iterator iterator;
    typedef std::set<EqClass *>::const_iterator const_iterator;
    typedef std::set<EqClass *>::reverse_iterator reverse_iterator;
    typedef std::set<EqClass *>::const_reverse_iterator const_reverse_iterator;

    EqClasses() = default;
    EqClasses(const EqClasses&) = delete;
    EqClasses(EqClasses&&) = default;
    EqClasses(EqClass *);
    ~EqClasses();

    EqClasses& operator=(const EqClasses&) = delete;
    EqClasses& operator=(EqClasses&&) = default;

    void add_ec(const ECRange&);
    void add_ec(const IPNetwork<IPv4Address>&);
    void add_ec(const IPv4Address&);
    void add_mask_range(const ECRange&, const EqClasses&);

    std::string to_string() const;
    bool empty() const;
    size_type size() const;
    EqClass *find_ec(const IPv4Address&) const;
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
