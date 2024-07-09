#pragma once

#include "lib/ip.hpp"
class ECRange;
#include "eqclass.hpp"

/*
 * ECRange represents a continuous range of packets (classified by the
 * destination IP addresses) within the same equivalence class. Different
 * ECRange instances may belong to the same EqClass, but each ECRange instance
 * belongs only to one EqClass.
 */
class ECRange : public IPRange<IPv4Address> {
private:
    EqClass *ec;

    friend bool operator<(const ECRange &, const ECRange &);
    friend bool operator<=(const ECRange &, const ECRange &);
    friend bool operator>(const ECRange &, const ECRange &);
    friend bool operator>=(const ECRange &, const ECRange &);
    friend bool operator==(const ECRange &, const ECRange &);
    friend bool operator!=(const ECRange &, const ECRange &);

public:
    ECRange()                = delete;
    ECRange(const ECRange &) = default;
    ECRange(ECRange &&)      = default;
    ECRange(const IPv4Address &lb, const IPv4Address &ub);
    ECRange(const IPNetwork<IPv4Address> &);
    ECRange(const IPRange<IPv4Address> &);

    void set_ec(EqClass *);
    EqClass *get_ec() const;
    bool identical_to(const ECRange &) const; // having the same range
    bool contains(const IPv4Address &) const;
    bool contains(const ECRange &) const;
    bool contains(const EqClass &) const;

    ECRange &operator=(const ECRange &) = default;
    ECRange &operator=(ECRange &&)      = default;
};

bool operator<(const ECRange &, const ECRange &);
bool operator<=(const ECRange &, const ECRange &);
bool operator>(const ECRange &, const ECRange &);
bool operator>=(const ECRange &, const ECRange &);
bool operator==(const ECRange &, const ECRange &); // there is an overlap
bool operator!=(const ECRange &, const ECRange &); // there is no overlap
