#include "eqclass.hpp"

std::string EqClass::to_string() const {
    std::string ret = "[";
    for (const ECRange &range : ranges) {
        ret += " " + range.to_string();
    }
    ret += " ]";
    return ret;
}

bool EqClass::empty() const {
    return ranges.empty();
}

void EqClass::add_range(const ECRange &range) {
    ranges.insert(range);
}

void EqClass::rm_range(const ECRange &range) {
    while (ranges.erase(range) > 0)
        ;
}

bool EqClass::contains(const IPv4Address &addr) const {
    for (const ECRange &range : ranges) {
        if (range.contains(addr)) {
            return true;
        }
    }
    return false;
}

IPv4Address EqClass::representative_addr() const {
    return ranges.begin()->get_lb();
}

EqClass::iterator EqClass::begin() {
    return ranges.begin();
}

EqClass::const_iterator EqClass::begin() const {
    return ranges.begin();
}

EqClass::iterator EqClass::end() {
    return ranges.end();
}

EqClass::const_iterator EqClass::end() const {
    return ranges.end();
}

EqClass::reverse_iterator EqClass::rbegin() {
    return ranges.rbegin();
}

EqClass::const_reverse_iterator EqClass::rbegin() const {
    return ranges.rbegin();
}

EqClass::reverse_iterator EqClass::rend() {
    return ranges.rend();
}

EqClass::const_reverse_iterator EqClass::rend() const {
    return ranges.rend();
}

bool operator==(const EqClass &a, const EqClass &b) {
    if (a.ranges.size() != b.ranges.size()) {
        return false;
    }

    std::set<ECRange>::const_iterator a_i = a.begin(), b_i = b.begin();
    while (a_i != a.end()) {
        if (!a_i->identical_to(*b_i)) {
            return false;
        }
        ++a_i;
        ++b_i;
    }
    return true;
}
