#include "eqclass.hpp"

std::string EqClass::to_string() const
{
    std::string ret = "[";
    for (const ECRange& range : ranges) {
        ret += " " + range.to_string();
    }
    ret += " ]";
    return ret;
}

bool EqClass::empty() const
{
    return ranges.empty();
}

const std::set<ECRange>& EqClass::get_ranges() const
{
    return ranges;
}

std::set<ECRange>::iterator EqClass::add_range(const ECRange& range)
{
    return ranges.insert(range).first;
}

void EqClass::rm_range(const ECRange& range)
{
    while (ranges.erase(range) > 0);
}
