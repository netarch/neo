#pragma once

#include <string>
#include <map>
#include <set>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "interface.hpp"
#include "route.hpp"

class Node
{
protected:
    std::string name;
    std::string type;

    // interfaces indexed by name and ip address
    std::map<std::string, std::shared_ptr<Interface> > intfs;
    std::map<IPv4Address, std::shared_ptr<Interface> > intfs_ipv4;

    std::set<Route> static_routes;
    // () links;

    // rib

public:
    Node(const std::string&, const std::string&);

    virtual std::string get_name() const;
    virtual void load_interfaces(const std::shared_ptr<cpptoml::table_array>);
    virtual void load_static_routes(
        const std::shared_ptr<cpptoml::table_array>);
};
