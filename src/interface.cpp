#include "interface.hpp"
#include "lib/logger.hpp"

Interface::Interface(const std::string& name): name(name), switchport(true)
{
}

Interface::Interface(const std::string& name, const std::string& ip)
    : name(name), ipv4(ip), switchport(false)
{
}

std::string Interface::to_string() const
{
    return name;
}

std::string Interface::get_name() const
{
    return name;
}

IPv4Address Interface::addr() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.addr();
}

int Interface::prefix_length() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.prefix_length();
}

IPNetwork<IPv4Address> Interface::network() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.network();
}

bool Interface::switching() const
{
    return switchport;
}
