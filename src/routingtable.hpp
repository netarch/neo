#pragma once

#include <set>
#include <utility>
#include <string>

#include "route.hpp"

class RoutingTable
{
private:
    std::multiset<Route> tbl;

public:
    typedef std::multiset<Route>::size_type size_type;
    typedef std::multiset<Route>::iterator iterator;
    typedef std::multiset<Route>::const_iterator const_iterator;
    typedef std::multiset<Route>::reverse_iterator reverse_iterator;
    typedef std::multiset<Route>::const_reverse_iterator const_reverse_iterator;

    RoutingTable() = default;
    RoutingTable(const RoutingTable&) = default;
    RoutingTable(RoutingTable&&) = default;
    RoutingTable& operator=(const RoutingTable&) = default;
    RoutingTable& operator=(RoutingTable&&) = default;

    std::string to_string() const;

    iterator insert(const Route&);
    iterator insert(Route&&);
    size_type erase(const Route&);
    iterator erase(const_iterator);
    void clear();
    template <class... Args> iterator emplace(const Args& ... args);
    template <class... Args> iterator emplace(Args&& ... args);

    /*
     * lookup returns the range of the elements equal to the given route.
     * The first iterator points to the first element in the range, and the
     * second iterator points to one after the last element in the range, i.e.
     * [first, second). If there's no such route in the routing table, the range
     * returned has a length of zero, with both iterators pointing to the first
     * element that goes after the given route.
     */
    std::pair<iterator, iterator>
    lookup(const IPNetwork<IPv4Address>&);
    std::pair<const_iterator, const_iterator>
    lookup(const IPNetwork<IPv4Address>&) const;
    std::pair<iterator, iterator>
    lookup(const IPv4Address&);
    std::pair<const_iterator, const_iterator>
    lookup(const IPv4Address&) const;

    size_type count(const IPNetwork<IPv4Address>&) const;
    size_type count(const Route&) const;

    bool empty() const;
    size_type size() const;

    iterator               begin();
    const_iterator         begin() const;
    iterator               end();
    const_iterator         end() const;
    reverse_iterator       rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator       rend();
    const_reverse_iterator rend() const;
};

template <class... Args>
RoutingTable::iterator RoutingTable::emplace(const Args& ... args)
{
    return insert(Route(args...));
}

template <class... Args>
RoutingTable::iterator RoutingTable::emplace(Args&& ... args)
{
    return insert(Route(args...));
}
