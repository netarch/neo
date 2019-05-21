#pragma once

#include <string>
#include <map>
#include <memory>

#include "interface.hpp"
#include "lib/cpptoml.hpp"

class Node
{
protected:
    std::string name;
    std::string type;

    // interfaces indexed by name and ip address
    std::map<std::string, std::shared_ptr<Interface> > intfs;
    std::map<IPv4Address, std::shared_ptr<Interface> > intfs_ipv4;

    // (set of Routes) static_routes;
    // () links;

    // rib

public:
    Node(const std::string&, const std::string&);

    virtual std::string get_name() const;
    virtual void add_interface(const std::shared_ptr<Interface>&);
    virtual void load_interfaces(const std::shared_ptr<cpptoml::table_array>);
    virtual void load_static_routes(const std::shared_ptr<cpptoml::table_array>);
};
