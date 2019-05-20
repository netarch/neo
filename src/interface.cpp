#include "interface.hpp"
#include "logger.hpp"

Interface::Interface(const std::string& name): name(name), L3(false)
{
}

Interface::Interface(const std::string& name, const std::string& intf)
    : name(name), ipv4(intf), L3(true)
{
}

std::string Interface::get_name() const
{
    return name;
}

IPv4Address Interface::addr() const
{
    if (!L3) {
        Logger::get_instance().err("Switching port: " + name);
    }
    return ipv4.addr();
}

int Interface::prefix_length() const
{
    if (!L3) {
        Logger::get_instance().err("Switching port: " + name);
    }
    return ipv4.prefix_length();
}

IPNetwork<IPv4Address> Interface::network() const
{
    if (!L3) {
        Logger::get_instance().err("Switching port: " + name);
    }
    return ipv4.network();
}

bool Interface::switching() const
{
    return !L3;
}
