#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>

class Interface
{
private:
    std::string name;
    //ip;

public:
    Interface(const std::string& name);
    //Interface(const std::string& name, ip);
};

#endif
