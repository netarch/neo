#include <utility>

#include "link.hpp"
#include "lib/logger.hpp"

void Link::normalize()
{
    if (node1 > node2) {
        std::swap(node1, node2);
        std::swap(intf1, intf2);
    } else if (node1 == node2) {
        Logger::get_instance().err("Invalid link: " + to_string());
    }
}

Link::Link(const std::shared_ptr<Node>& node1,
           const std::shared_ptr<Interface>& intf1,
           const std::shared_ptr<Node>& node2,
           const std::shared_ptr<Interface>& intf2)
    : node1(node1), intf1(intf1), node2(node2), intf2(intf2), failed(false)
{
    normalize();
}

std::string Link::to_string() const
{
    return node1->to_string() + ":" + intf1->to_string() + " --- " +
           node2->to_string() + ":" + intf2->to_string();
}

std::shared_ptr<Node> Link::get_node1() const
{
    return node1;
}

std::shared_ptr<Node> Link::get_node2() const
{
    return node2;
}

std::shared_ptr<Interface> Link::get_intf1() const
{
    return intf1;
}

std::shared_ptr<Interface> Link::get_intf2() const
{
    return intf2;
}

bool Link::fail() const
{
    return failed;
}

void Link::set_fail(bool f)
{
    failed = f;
}

bool Link::operator<(const Link& rhs) const
{
    if (node1 < rhs.node1) {
        return true;
    } else if (node1 > rhs.node1) {
        return false;
    }

    if (intf1 < rhs.intf1) {
        return true;
    } else if (intf1 > rhs.intf1) {
        return false;
    }

    if (node2 < rhs.node2) {
        return true;
    } else if (node2 > rhs.node2) {
        return false;
    }

    if (intf2 < rhs.intf2) {
        return true;
    } else if (intf2 > rhs.intf2) {
        return false;
    }

    return false;
}

bool Link::operator<=(const Link& rhs) const
{
    return !(*this > rhs);
}

bool Link::operator>(const Link& rhs) const
{
    if (node1 > rhs.node1) {
        return true;
    } else if (node1 < rhs.node1) {
        return false;
    }

    if (intf1 > rhs.intf1) {
        return true;
    } else if (intf1 < rhs.intf1) {
        return false;
    }

    if (node2 > rhs.node2) {
        return true;
    } else if (node2 < rhs.node2) {
        return false;
    }

    if (intf2 > rhs.intf2) {
        return true;
    } else if (intf2 < rhs.intf2) {
        return false;
    }

    return false;
}

bool Link::operator>=(const Link& rhs) const
{
    return !(*this < rhs);
}

bool Link::operator==(const Link& rhs) const
{
    if (node1 == rhs.node1 && intf1 == rhs.intf1 &&
            node2 == rhs.node2 && intf2 == rhs.intf2) {
        return true;
    }
    return false;
}

bool Link::operator!=(const Link& rhs) const
{
    return !(*this == rhs);
}

Link& Link::operator=(const Link& rhs)
{
    node1 = rhs.node1;
    node2 = rhs.node2;
    intf1 = rhs.intf1;
    intf2 = rhs.intf2;
    failed = rhs.failed;
    return *this;
}
