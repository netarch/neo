#include "injection-result.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <vector>

#include "lib/hash.hpp"

InjectionResult::InjectionResult(std::list<Packet> &&recv_pkts,
                                 bool explicit_drop) :
    _explicit_drop(explicit_drop) {
    std::move(recv_pkts.begin(), recv_pkts.end(),
              std::back_inserter(_recv_pkts));
    sort(_recv_pkts.begin(), _recv_pkts.end());
    _recv_pkts.erase(unique(_recv_pkts.begin(), _recv_pkts.end()),
                     _recv_pkts.end());
}

std::string InjectionResult::to_string() const {
    std::string res = "---";
    for (const Packet &p : _recv_pkts) {
        res += "\n - Received: " + p.to_string();
    }
    res += "\n - Explicitly dropped: " +
           std::string(_explicit_drop ? "true" : "false");
    return res;
}

bool operator<(const InjectionResult &a, const InjectionResult &b) {
    if (a._recv_pkts < b._recv_pkts) {
        return true;
    }
    if (a._explicit_drop < b._explicit_drop) {
        return true;
    }
    return false;
}

size_t InjectionResultHash::operator()(const InjectionResult *const &ir) const {
    size_t value = 0;
    PacketHash pkt_hash;
    for (const Packet &p : ir->_recv_pkts) {
        hash::hash_combine(value, pkt_hash(p));
    }
    hash::hash_combine(value, std::hash<bool>()(ir->_explicit_drop));
    return value;
}

bool InjectionResultEq::operator()(const InjectionResult *const &a,
                                   const InjectionResult *const &b) const {
    return a->_recv_pkts == b->_recv_pkts &&
           a->_explicit_drop == b->_explicit_drop;
}

std::string InjectionResults::to_string() const {
    std::string res = "Injection results:";
    for (InjectionResult *result : _results) {
        res += "\n" + result->to_string();
    }
    return res;
}

void InjectionResults::add(InjectionResult *result) {
    _results.insert(std::upper_bound(_results.begin(), _results.end(), result),
                    result);
    _results.erase(unique(_results.begin(), _results.end()), _results.end());
}

size_t InjectionResults::size() const {
    return _results.size();
}

const InjectionResult &InjectionResults::at(size_t i) const {
    return *_results.at(i);
}

size_t
InjectionResultsHash::operator()(const InjectionResults *const &results) const {
    return hash::hash(results->_results.data(),
                      results->_results.size() * sizeof(InjectionResult *));
}

bool InjectionResultsEq::operator()(const InjectionResults *const &a,
                                    const InjectionResults *const &b) const {
    if (a->_results.size() != b->_results.size()) {
        return false;
    }
    return memcmp(a->_results.data(), b->_results.data(),
                  a->_results.size() * sizeof(InjectionResult *)) == 0;
}
