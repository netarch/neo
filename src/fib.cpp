#include <functional>

#include "fib.hpp"

void FIB::set_next_hops(const std::shared_ptr<Node>& node,
                        std::set<std::shared_ptr<Node> >&& next_hops)
{
    tbl[node] = next_hops;
}

void FIB::set_next_hops(std::shared_ptr<Node>&& node,
                        std::set<std::shared_ptr<Node> >&& next_hops)
{
    tbl[node] = next_hops;
}

const std::set<std::shared_ptr<Node> >& FIB::get_next_hops(
    const std::shared_ptr<Node>& node) const
{
    return tbl.at(node);
}

const std::set<std::shared_ptr<Node> >& FIB::get_next_hops(
    std::shared_ptr<Node>&& node) const
{
    return tbl.at(node);
}

std::string FIB::to_string() const
{
    std::string ret = "FIB:\n";
    for (const auto& entry : tbl) {
        ret += entry.first->to_string() + " -> [";
        std::set<std::shared_ptr<Node> >::const_iterator it
            = entry.second.begin();
        if (it != entry.second.end()) {
            ret += (*it)->to_string();
            ++it;
        }
        while (it != entry.second.end()) {
            ret += ", " + (*it)->to_string();
            ++it;
        }
        ret += "]\n";
    }
    return ret;
}

bool FIB::empty() const
{
    return tbl.empty();
}

FIB::size_type FIB::size() const
{
    return tbl.size();
}

bool FIB::operator==(const FIB& rhs) const
{
    return tbl == rhs.tbl;
}

FIB::iterator FIB::begin()
{
    return tbl.begin();
}

FIB::const_iterator FIB::begin() const
{
    return tbl.begin();
}

FIB::iterator FIB::end()
{
    return tbl.end();
}

FIB::const_iterator FIB::end() const
{
    return tbl.end();
}

FIB::reverse_iterator FIB::rbegin()
{
    return tbl.rbegin();
}

FIB::const_reverse_iterator FIB::rbegin() const
{
    return tbl.rbegin();
}

FIB::reverse_iterator FIB::rend()
{
    return tbl.rend();
}

FIB::const_reverse_iterator FIB::rend() const
{
    return tbl.rend();
}

size_t FIBHash::operator()(const std::shared_ptr<FIB>& fib) const
{
    size_t hash = 0;
    for (const auto& entry : fib->tbl) {
        hash <<= 1;
        for (const auto& nh : entry.second) {
            hash ^= std::hash<std::shared_ptr<Node> >()(nh);
        }
    }
    return hash;
}

bool FIBEqual::operator()(const std::shared_ptr<FIB>& fib1,
                          const std::shared_ptr<FIB>& fib2) const
{
    return *fib1 == *fib2;
}
