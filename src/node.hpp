#ifndef NODE_HPP
#define NODE_HPP

#include <string>
#include <map>
#include <memory>

#include "interface.hpp"
#include "lib/cpptoml.hpp"

class Node
{
private:
    std::string name;
    std::string type;

    // interfaces indexed by name and ip address
    std::map<std::string, std::shared_ptr<Interface> > intfs;
    std::map<IPv4Address, std::shared_ptr<Interface> > intfs_ipv4;

    // (sorted list by longest prefix, then numerical order) static_routes;
    // () links;

    //fib

public:
    Node(const std::string&, const std::string&);

    virtual std::string get_name() const;
    virtual void add_interface(const std::shared_ptr<Interface>&);
    virtual void load_interfaces(const std::shared_ptr<cpptoml::table_array>);
    //virtual void load_static_routes(const std::shared_ptr<cpptoml::table_array>);
};

#endif
