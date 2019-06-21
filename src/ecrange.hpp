#pragma once

#include <memory>

#include "lib/ip.hpp"
class ECRange;
#include "eqclass.hpp"

/*
 * ECRange represents a continuous range of packets (classified by the
 * destination IP addresses) within the same equivalence class. Different
 * ECRange instances may belong to the same EqClass, but each ECRange instance
 * belongs only to one EqClass.
 */
class ECRange : public IPRange<IPv4Address>
{
private:
    std::weak_ptr<EqClass> EC;

public:
    ECRange() = delete;
    ECRange(const ECRange&) = default;
    ECRange(ECRange&&) = default;
    ECRange(const IPv4Address& lb, const IPv4Address& ub);
    ECRange(const IPNetwork<IPv4Address>&);
    ECRange(const IPRange<IPv4Address>&);

    void set_ec(const std::shared_ptr<EqClass>&);
    std::shared_ptr<EqClass> get_ec() const;

    bool operator< (const ECRange&) const;
    bool operator<=(const ECRange&) const;
    bool operator> (const ECRange&) const;
    bool operator>=(const ECRange&) const;
    bool operator==(const ECRange&) const;  // there is an overlap
    bool operator!=(const ECRange&) const;  // there is no overlap
    bool identical_to(const ECRange&) const;    // having the same range
    bool contains(const ECRange&) const;
    bool contains(const EqClass&) const;

    ECRange& operator=(const ECRange&) = default;
    ECRange& operator=(ECRange&&) = default;
};
