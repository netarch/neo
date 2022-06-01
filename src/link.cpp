#include "link.hpp"

#include <utility>

#include "lib/logger.hpp"

std::string Link::to_string() const {
    return node1->to_string() + ":" + intf1->to_string() + " --- " +
           node2->to_string() + ":" + intf2->to_string();
}

Node *Link::get_node1() const {
    return node1;
}

Node *Link::get_node2() const {
    return node2;
}

Interface *Link::get_intf1() const {
    return intf1;
}

Interface *Link::get_intf2() const {
    return intf2;
}

bool operator<(const Link &a, const Link &b) {
    if (a.node1 < b.node1) {
        return true;
    } else if (a.node1 > b.node1) {
        return false;
    }

    if (a.intf1 < b.intf1) {
        return true;
    } else if (a.intf1 > b.intf1) {
        return false;
    }

    if (a.node2 < b.node2) {
        return true;
    } else if (a.node2 > b.node2) {
        return false;
    }

    if (a.intf2 < b.intf2) {
        return true;
    } else if (a.intf2 > b.intf2) {
        return false;
    }

    return false;
}

bool operator<=(const Link &a, const Link &b) {
    return !(a > b);
}

bool operator>(const Link &a, const Link &b) {
    if (a.node1 > b.node1) {
        return true;
    } else if (a.node1 < b.node1) {
        return false;
    }

    if (a.intf1 > b.intf1) {
        return true;
    } else if (a.intf1 < b.intf1) {
        return false;
    }

    if (a.node2 > b.node2) {
        return true;
    } else if (a.node2 < b.node2) {
        return false;
    }

    if (a.intf2 > b.intf2) {
        return true;
    } else if (a.intf2 < b.intf2) {
        return false;
    }

    return false;
}

bool operator>=(const Link &a, const Link &b) {
    return !(a < b);
}

bool operator==(const Link &a, const Link &b) {
    if (a.node1 == b.node1 && a.intf1 == b.intf1 && a.node2 == b.node2 &&
        a.intf2 == b.intf2) {
        return true;
    }
    return false;
}

bool operator!=(const Link &a, const Link &b) {
    return !(a == b);
}

bool LinkCompare::operator()(Link *const link1, Link *const link2) const {
    return *link1 < *link2;
}
