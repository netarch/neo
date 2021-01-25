#include "candidates.hpp"

#include <cstring>

#include "lib/hash.hpp"

void Candidates::add(const FIB_IPNH& next_hop)
{
    this->next_hops.push_back(next_hop);
}

size_t Candidates::size() const
{
    return this->next_hops.size();
}

const FIB_IPNH& Candidates::at(size_t idx) const
{
    return this->next_hops.at(idx);
}

size_t CandHash::operator()(const Candidates *const& c) const
{
    return hash::hash(c->next_hops.data(), c->next_hops.size() * sizeof(FIB_IPNH));
}

bool CandEq::operator()(const Candidates *const& a, const Candidates *const& b) const
{
    if (a->next_hops.size() != b->next_hops.size()) {
        return false;
    }
    return memcmp(a->next_hops.data(), b->next_hops.data(),
                  a->next_hops.size() * sizeof(FIB_IPNH)) == 0;
}
