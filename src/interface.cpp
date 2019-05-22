#include "interface.hpp"
#include "lib/logger.hpp"

Interface::Interface(const std::string& name): name(name), switchport(true)
{
}

Interface::Interface(const std::string& name, const std::string& intf)
    : name(name), ipv4(intf), switchport(false)
{
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
