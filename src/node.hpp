#pragma once

#include <string>
#include <map>
#include <set>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "interface.hpp"
#include "route.hpp"
class Node;
#include "link.hpp"

class Node
{
protected:
    std::string name;
    std::string type;

    // interfaces indexed by name and ip address
    std::map<std::string, std::shared_ptr<Interface> > intfs;
    std::map<IPv4Address, std::shared_ptr<Interface> > intfs_ipv4;

    // active connected peers indexed by interface name
    std::map<std::string,
        std::pair<std::shared_ptr<Node>, std::shared_ptr<Interface> >
        > active_peers;

    // active links indexed by interface name
    std::map<std::string, std::shared_ptr<Link> > active_links;

    std::set<Route> static_routes;
    std::set<Route> rib;    // global RIB for this node

    void rib_install(const Route&);

public:
    Node() = delete;
    Node(const Node&) = default;
    Node(const std::string&, const std::string&);

    virtual std::string to_string() const;
    virtual std::string get_name() const;
    virtual const std::shared_ptr<Interface>&
    get_interface(const std::string&) const;
    virtual const std::shared_ptr<Interface>&
    get_interface(const IPv4Address&) const;

    virtual void
    load_interfaces(const std::shared_ptr<cpptoml::table_array>);
    virtual void
    load_static_routes(const std::shared_ptr<cpptoml::table_array>);
    virtual void
    load_installed_routes(const std::shared_ptr<cpptoml::table_array>);

    virtual void add_peer(const std::string&, const std::shared_ptr<Node>&,
                          const std::shared_ptr<Interface>&);
    virtual void add_link(const std::string&, const std::shared_ptr<Link>&);
};
