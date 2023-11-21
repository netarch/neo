#pragma once

#include <list>
#include <string>
#include <vector>

#include "packet.hpp"

class InjectionResult {
private:
    std::vector<Packet> _recv_pkts;
    bool _explicit_drop = false;

    friend class InjectionResultHash;
    friend class InjectionResultEq;
    friend bool operator<(const InjectionResult &, const InjectionResult &);

public:
    InjectionResult() = default;
    InjectionResult(std::list<Packet> &&recv_pkts, bool explicit_drop);

    std::string to_string() const;
    const decltype(_recv_pkts) &recv_pkts() const { return _recv_pkts; }
    decltype(_explicit_drop) explicit_drop() const { return _explicit_drop; }
};

bool operator<(const InjectionResult &, const InjectionResult &);

class InjectionResultHash {
public:
    size_t operator()(const InjectionResult *const &) const;
};

class InjectionResultEq {
public:
    bool operator()(const InjectionResult *const &,
                    const InjectionResult *const &) const;
};

class InjectionResults {
private:
    // All elements in the vector should be sorted and unique.
    // Since we expect the number of duplicate injection results will be small,
    // using a vector should be more performant.
    std::vector<InjectionResult *> _results;

    friend class InjectionResultsHash;
    friend class InjectionResultsEq;

public:
    std::string to_string() const;
    void add(InjectionResult *);
    size_t size() const;
    const InjectionResult &at(size_t) const;
};

class InjectionResultsHash {
public:
    size_t operator()(const InjectionResults *const &) const;
};

class InjectionResultsEq {
public:
    bool operator()(const InjectionResults *const &,
                    const InjectionResults *const &) const;
};
