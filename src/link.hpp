#pragma once

#include <memory>
#include <string>
#include <cpptoml/cpptoml.hpp>

class Link;
#include "node.hpp"

class Link
{
private:
    std::weak_ptr<Node>      node1;
    std::weak_ptr<Interface> intf1;
    std::weak_ptr<Node>      node2;
    std::weak_ptr<Interface> intf2;
    bool failed;

    void normalize();   // called by every user-defined constructor

public:
    Link(const Link&) = default;
    Link(const std::shared_ptr<cpptoml::table>& config,
         const std::map<std::string, std::shared_ptr<Node> >& nodes);

    std::string to_string() const;
    std::shared_ptr<Node> get_node1() const;
    std::shared_ptr<Node> get_node2() const;
    std::shared_ptr<Interface> get_intf1() const;
    std::shared_ptr<Interface> get_intf2() const;
    bool fail() const;
    void set_fail(bool);

    bool operator< (const Link&) const;
    bool operator<=(const Link&) const;
    bool operator> (const Link&) const;
    bool operator>=(const Link&) const;
    bool operator==(const Link&) const;
    bool operator!=(const Link&) const;

    Link& operator=(const Link&) = default;
    Link& operator=(Link&&) = default;
};

class LinkCompare
{
public:
    bool operator()(const std::shared_ptr<Link>&,
                    const std::shared_ptr<Link>&) const;
};
