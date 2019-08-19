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

    auto node1_entry = nodes.find(*node1_name);
    if (node1_entry == nodes.end()) {
        Logger::get_instance().err("Unknown node: " + *node1_name);
    }
    node1 = node1_entry->second;
    auto node2_entry = nodes.find(*node2_name);
    if (node2_entry == nodes.end()) {
        Logger::get_instance().err("Unknown node: " + *node2_name);
    }
    node2 = node2_entry->second;
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

bool operator<(const Link& a, const Link& b)
{
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

bool operator<=(const Link& a, const Link& b)
{
    return !(a > b);
}

bool operator>(const Link& a, const Link& b)
{
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

bool operator>=(const Link& a, const Link& b)
{
    return !(a < b);
}

bool operator==(const Link& a, const Link& b)
{
    if (a.node1 == b.node1 && a.intf1 == b.intf1 &&
            a.node2 == b.node2 && a.intf2 == b.intf2) {
        return true;
    }
    return false;
}

bool operator!=(const Link& a, const Link& b)
{
    return !(a == b);
}

bool LinkCompare::operator()(Link *const link1, Link *const link2) const
{
    return *link1 < *link2;
}
