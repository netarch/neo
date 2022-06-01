#include "interface.hpp"

#include "lib/logger.hpp"

std::string Interface::to_string() const {
    return name;
}

std::string Interface::get_name() const {
    return name;
}

int Interface::prefix_length() const {
    if (is_switchport) {
        Logger::error("Switchport: " + name);
    }
    return ipv4.prefix_length();
}

IPv4Address Interface::addr() const {
    if (is_switchport) {
        Logger::error("Switchport: " + name);
    }
    return ipv4.addr();
}

IPv4Address Interface::mask() const {
    if (is_switchport) {
        Logger::error("Switchport: " + name);
    }
    return ipv4.mask();
}

IPNetwork<IPv4Address> Interface::network() const {
    if (is_switchport) {
        Logger::error("Switchport: " + name);
    }
    return ipv4.network();
}

bool Interface::is_l2() const {
    return is_switchport;
}
