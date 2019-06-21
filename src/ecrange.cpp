#include "ecrange.hpp"

ECRange::ECRange(const IPv4Address& lb, const IPv4Address& ub)
    : IPRange<IPv4Address>(lb, ub)
{
}

ECRange::ECRange(const IPNetwork<IPv4Address>& net)
    : IPRange<IPv4Address>(net)
{
}

ECRange::ECRange(const IPRange<IPv4Address>& range)
    : IPRange<IPv4Address>(range)
{
}

void ECRange::set_ec(const std::shared_ptr<EqClass>& ec)
{
    EC = ec;
}

std::shared_ptr<EqClass> ECRange::get_ec() const
{
    return EC.lock();
}

bool ECRange::operator<(const ECRange& rhs) const
{
    return ub < rhs.lb;
}

bool ECRange::operator<=(const ECRange& rhs) const
{
    return !(*this > rhs);
}

bool ECRange::operator>(const ECRange& rhs) const
{
    return lb > rhs.ub;
}

bool ECRange::operator>=(const ECRange& rhs) const
{
    return !(*this < rhs);
}

bool ECRange::operator==(const ECRange& rhs) const
{
    return (*this <= rhs && *this >= rhs);
}

bool ECRange::operator!=(const ECRange& rhs) const
{
    return (*this < rhs || *this > rhs);
}

bool ECRange::identical_to(const ECRange& rhs) const
{
    return (lb == rhs.lb && ub == rhs.ub);
}

bool ECRange::contains(const ECRange& rhs) const
{
    return (lb <= rhs.lb && rhs.ub <= ub);
}

bool ECRange::contains(const EqClass& rhs) const
{
    return (lb <= rhs.get_ranges().cbegin()->lb &&
            rhs.get_ranges().crbegin()->ub <= ub);
}
