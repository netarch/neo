#pragma once

#include <memory>
#include <string>

#include "node.hpp"

class Link {
private:
    Node *node1;
    Node *node2;
    Interface *intf1;
    Interface *intf2;

    friend bool operator<(const Link &, const Link &);
    friend bool operator<=(const Link &, const Link &);
    friend bool operator>(const Link &, const Link &);
    friend bool operator>=(const Link &, const Link &);
    friend bool operator==(const Link &, const Link &);
    friend bool operator!=(const Link &, const Link &);

private:
    friend class Config;
    Link() = default;

public:
    std::string to_string() const;
    Node *get_node1() const;
    Node *get_node2() const;
    Interface *get_intf1() const;
    Interface *get_intf2() const;
};

bool operator<(const Link &, const Link &);
bool operator<=(const Link &, const Link &);
bool operator>(const Link &, const Link &);
bool operator>=(const Link &, const Link &);
bool operator==(const Link &, const Link &);
bool operator!=(const Link &, const Link &);

class LinkCompare {
public:
    bool operator()(Link *const, Link *const) const;
};
