#include <utility>

#include "link.hpp"
#include "lib/logger.hpp"

void Link::normalize()
{
    if (node1.lock() > node2.lock()) {
        std::swap(node1, node2);
        std::swap(intf1, intf2);
    } else if (node1.lock() == node2.lock()) {
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
    return node1.lock()->to_string() + ":" + intf1.lock()->to_string() +
           " --- " +
           node2.lock()->to_string() + ":" + intf2.lock()->to_string();
}

std::shared_ptr<Node> Link::get_node1() const
{
    return node1.lock();
}

std::shared_ptr<Node> Link::get_node2() const
{
    return node2.lock();
}

std::shared_ptr<Interface> Link::get_intf1() const
{
    return intf1.lock();
}

std::shared_ptr<Interface> Link::get_intf2() const
{
    return intf2.lock();
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
    if (node1.lock() < rhs.node1.lock()) {
        return true;
    } else if (node1.lock() > rhs.node1.lock()) {
        return false;
    }

    if (intf1.lock() < rhs.intf1.lock()) {
        return true;
    } else if (intf1.lock() > rhs.intf1.lock()) {
        return false;
    }

    if (node2.lock() < rhs.node2.lock()) {
        return true;
    } else if (node2.lock() > rhs.node2.lock()) {
        return false;
    }

    if (intf2.lock() < rhs.intf2.lock()) {
        return true;
    } else if (intf2.lock() > rhs.intf2.lock()) {
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
    if (node1.lock() > rhs.node1.lock()) {
        return true;
    } else if (node1.lock() < rhs.node1.lock()) {
        return false;
    }

    if (intf1.lock() > rhs.intf1.lock()) {
        return true;
    } else if (intf1.lock() < rhs.intf1.lock()) {
        return false;
    }

    if (node2.lock() > rhs.node2.lock()) {
        return true;
    } else if (node2.lock() < rhs.node2.lock()) {
        return false;
    }

    if (intf2.lock() > rhs.intf2.lock()) {
        return true;
    } else if (intf2.lock() < rhs.intf2.lock()) {
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
    if (node1.lock() == rhs.node1.lock() &&
            intf1.lock() == rhs.intf1.lock() &&
            node2.lock() == rhs.node2.lock() &&
            intf2.lock() == rhs.intf2.lock()) {
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
