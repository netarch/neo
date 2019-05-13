#ifndef NODE_HPP
#define NODE_HPP

#include <string>

class Node
{
private:
    std::string name;
    std::string type;
    //interfaces;
    //static_routes;
    //links;

    //fib

public:
    Node(const std::string&, const std::string&);
};

#endif
