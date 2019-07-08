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

Link::Link(const std::shared_ptr<cpptoml::table>& config,
           const std::map<std::string, std::shared_ptr<Node> >& nodes)
    : failed(false)
{
    auto node1_name = config->get_as<std::string>("node1");
    auto intf1_name = config->get_as<std::string>("intf1");
    auto node2_name = config->get_as<std::string>("node2");
    auto intf2_name = config->get_as<std::string>("intf2");

    if (!node1_name) {
        Logger::get_instance().err("Missing node1");
    }
    if (!intf1_name) {
        Logger::get_instance().err("Missing intf1");
    }
    if (!node2_name) {
        Logger::get_instance().err("Missing node2");
    }
    if (!intf2_name) {
        Logger::get_instance().err("Missing intf2");
    }

    node1 = nodes.at(*node1_name);
    node2 = nodes.at(*node2_name);
    intf1 = node1.lock()->get_interface(*intf1_name);
    intf2 = node2.lock()->get_interface(*intf2_name);

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

bool LinkCompare::operator()(const std::shared_ptr<Link>& link1,
                             const std::shared_ptr<Link>& link2) const
{
    return *link1 < *link2;
}
