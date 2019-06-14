#include <iterator>

#include "routingtable.hpp"

RoutingTable::iterator RoutingTable::insert(const Route& route)
{
    std::pair<iterator, iterator> range = tbl.equal_range(route);
    if (std::distance(range.first, range.second) > 0) {
        if (range.first->get_adm_dist() < route.get_adm_dist()) {
            return tbl.end();   // ignore the new route
        } else if (range.first->get_adm_dist() > route.get_adm_dist()) {
            tbl.erase(range.first, range.second);
        } else {    // having the same administrative distance
            for (iterator it = range.first; it != range.second; ++it) {
                if (it->has_same_path(route)) {
                    return it;
                }
            }
        }
    }
    return tbl.insert(route);
}

RoutingTable::iterator RoutingTable::insert(Route&& route)
{
    std::pair<iterator, iterator> range = tbl.equal_range(route);
    if (std::distance(range.first, range.second) > 0) {
        if (range.first->get_adm_dist() < route.get_adm_dist()) {
            return tbl.end();   // ignore the new route
        } else if (range.first->get_adm_dist() > route.get_adm_dist()) {
            tbl.erase(range.first, range.second);
        } else {    // having the same administrative distance
            for (iterator it = range.first; it != range.second; ++it) {
                if (it->has_same_path(route)) {
                    return it;
                }
            }
        }
    }
    return tbl.insert(route);
}

RoutingTable::size_type RoutingTable::erase(const Route& route)
{
    return tbl.erase(route);
}

RoutingTable::iterator RoutingTable::erase(const_iterator it)
{
    return tbl.erase(it);
}

void RoutingTable::clear()
{
    tbl.clear();
}

std::pair<RoutingTable::iterator, RoutingTable::iterator>
RoutingTable::lookup(const Route& route)
{
    return tbl.equal_range(route);
}

std::pair<RoutingTable::const_iterator, RoutingTable::const_iterator>
RoutingTable::lookup(const Route& route) const
{
    return tbl.equal_range(route);
}

RoutingTable::size_type RoutingTable::count(const Route& route) const
{
    return tbl.count(route);
}

bool RoutingTable::empty() const
{
    return tbl.empty();
}

RoutingTable::size_type RoutingTable::size() const
{
    return tbl.size();
}

RoutingTable::iterator RoutingTable::begin()
{
    return tbl.begin();
}

RoutingTable::const_iterator RoutingTable::begin() const
{
    return tbl.begin();
}

RoutingTable::iterator RoutingTable::end()
{
    return tbl.end();
}

RoutingTable::const_iterator RoutingTable::end() const
{
    return tbl.end();
}

RoutingTable::reverse_iterator RoutingTable::rbegin()
{
    return tbl.rbegin();
}

RoutingTable::const_reverse_iterator RoutingTable::rbegin() const
{
    return tbl.rbegin();
}

RoutingTable::reverse_iterator RoutingTable::rend()
{
    return tbl.rend();
}

RoutingTable::const_reverse_iterator RoutingTable::rend() const
{
    return tbl.rend();
}
