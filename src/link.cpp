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

Link::Link(const std::shared_ptr<cpptoml::table>& config,
           const std::map<std::string, Node *>& nodes)
//: failed(false)
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
    intf1 = node1->get_interface(*intf1_name);
    intf2 = node2->get_interface(*intf2_name);

    normalize();
}

std::string Link::to_string() const
{
    return node1->to_string() + ":" + intf1->to_string() +
           " --- " +
           node2->to_string() + ":" + intf2->to_string();
}

Node *Link::get_node1() const
{
    return node1;
}

Node *Link::get_node2() const
{
    return node2;
}

Interface *Link::get_intf1() const
{
    return intf1;
}

Interface *Link::get_intf2() const
{
    return intf2;
}

//bool Link::fail() const
//{
//    return failed;
//}
//
//void Link::set_fail(bool f)
//{
//    failed = f;
//}

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

bool LinkCompare::operator()(Link *const link1, Link *const link2) const
{
    return *link1 < *link2;
}
