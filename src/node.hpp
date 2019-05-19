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

    // interfaces indexed by name and ipv4
    std::map<std::string, std::shared_ptr<Interface> > interfaces;
    //std::map< ipv4 , std::shared_ptr<Interface> > interfaces_ipv4;

    // (sorted list by longest prefix, then numerical order) static_routes;
    // () links;

    //fib

public:
    Node(const std::string&, const std::string&);

    const std::string& get_name() const;

    //virtual void add_interface();
    virtual void load_interfaces(const std::shared_ptr<cpptoml::table_array>);
    //virtual void load_static_routes(const std::shared_ptr<cpptoml::table_array>);
};

#endif
