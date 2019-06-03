#pragma once

#include <memory>
#include <string>

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

    // called by every constructor except for copy constructor
    void normalize();

public:
    Link() = delete;
    Link(const Link&) = default;
    Link(const std::shared_ptr<Node>& node1,
         const std::shared_ptr<Interface>& intf1,
         const std::shared_ptr<Node>& node2,
         const std::shared_ptr<Interface>& intf2);

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

    Link& operator=(const Link&);
};
