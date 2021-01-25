#pragma once

#include <vector>

#include "fib.hpp"

class Candidates
{
private:
    std::vector<FIB_IPNH> next_hops;

    friend class CandHash;
    friend class CandEq;

public:
    void add(const FIB_IPNH&);
    size_t size() const;
    const FIB_IPNH& at(size_t) const;
};

class CandHash
{
public:
    size_t operator()(const Candidates *const&) const;
};

class CandEq
{
public:
    bool operator()(const Candidates *const&, const Candidates *const&) const;
};
