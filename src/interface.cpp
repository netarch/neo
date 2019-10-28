#include "interface.hpp"
#include "lib/logger.hpp"

Interface::Interface(const std::shared_ptr<cpptoml::table>& intf_config)
{
    auto intf_name = intf_config->get_as<std::string>("name");
    auto ipv4_addr = intf_config->get_as<std::string>("ipv4");
    // TODO vlans (ignored for now)

    if (!intf_name) {
        Logger::get_instance().err("Missing interface name");
    }
    name = *intf_name;
    if (ipv4_addr) {
        ipv4 = IPInterface<IPv4Address>(*ipv4_addr);
        switchport = false;
    } else {
        switchport = true;
    }
}

std::string Interface::to_string() const
{
    return name;
}

std::string Interface::get_name() const
{
    return name;
}

int Interface::prefix_length() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.prefix_length();
}

IPv4Address Interface::addr() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.addr();
}

IPv4Address Interface::mask() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.mask();
}

IPNetwork<IPv4Address> Interface::network() const
{
    if (switchport) {
        Logger::get_instance().err("Switchport: " + name);
    }
    return ipv4.network();
}

bool Interface::is_l2() const
{
    return switchport;
}
