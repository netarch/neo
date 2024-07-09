#include "ecrange.hpp"

ECRange::ECRange(const IPv4Address &lb, const IPv4Address &ub) :
    IPRange<IPv4Address>(lb, ub),
    ec(nullptr) {}

ECRange::ECRange(const IPNetwork<IPv4Address> &net) :
    IPRange<IPv4Address>(net),
    ec(nullptr) {}

ECRange::ECRange(const IPRange<IPv4Address> &range) :
    IPRange<IPv4Address>(range),
    ec(nullptr) {}

void ECRange::set_ec(EqClass *ec) {
    this->ec = ec;
}

EqClass *ECRange::get_ec() const {
    return ec;
}

bool ECRange::identical_to(const ECRange &rhs) const {
    return (lb == rhs.lb && ub == rhs.ub);
}

bool ECRange::contains(const IPv4Address &addr) const {
    return IPRange<IPv4Address>::contains(addr);
}

bool ECRange::contains(const ECRange &rhs) const {
    return (lb <= rhs.lb && rhs.ub <= ub);
}

bool ECRange::contains(const EqClass &rhs) const {
    return (lb <= rhs.begin()->lb && rhs.rbegin()->ub <= ub);
}

bool operator<(const ECRange &a, const ECRange &b) {
    return a.ub < b.lb;
}

bool operator<=(const ECRange &a, const ECRange &b) {
    return !(a > b);
}

bool operator>(const ECRange &a, const ECRange &b) {
    return a.lb > b.ub;
}

bool operator>=(const ECRange &a, const ECRange &b) {
    return !(a < b);
}

bool operator==(const ECRange &a, const ECRange &b) {
    return (a <= b && a >= b);
}

bool operator!=(const ECRange &a, const ECRange &b) {
    return (a < b || a > b);
}
