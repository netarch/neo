#pragma once

#include <string>
#include <cmath>

#include "lib/logger.hpp"

class IPv4Address
{
protected:
    uint32_t value;

public:
    IPv4Address() = default;
    IPv4Address(const IPv4Address&) = default;
    IPv4Address(const std::string&);
    IPv4Address(uint32_t);

    std::string to_string() const;
    size_t length() const;
    uint32_t get_value() const;
    bool operator< (const IPv4Address&) const;
    bool operator<=(const IPv4Address&) const;
    bool operator> (const IPv4Address&) const;
    bool operator>=(const IPv4Address&) const;
    bool operator==(const IPv4Address&) const;
    bool operator!=(const IPv4Address&) const;
    bool operator< (const std::string&) const;
    bool operator<=(const std::string&) const;
    bool operator> (const std::string&) const;
    bool operator>=(const std::string&) const;
    bool operator==(const std::string&) const;
    bool operator!=(const std::string&) const;
    IPv4Address& operator=(const IPv4Address&);
    IPv4Address& operator+=(const IPv4Address&);
    IPv4Address& operator+=(uint32_t);
    IPv4Address& operator-=(const IPv4Address&);
    IPv4Address& operator-=(uint32_t);
    IPv4Address& operator&=(const IPv4Address&);
    IPv4Address& operator&=(const std::string&);
    IPv4Address& operator|=(const IPv4Address&);
    IPv4Address& operator|=(const std::string&);
    IPv4Address operator+(const IPv4Address&) const;
    IPv4Address operator+(uint32_t) const;
    int         operator-(const IPv4Address&) const;
    int         operator-(uint32_t) const;
    IPv4Address operator&(const IPv4Address&) const;
    IPv4Address operator&(const std::string&) const;
    uint32_t    operator&(uint32_t) const;
    IPv4Address operator|(const IPv4Address&) const;
    IPv4Address operator|(const std::string&) const;
    uint32_t    operator|(uint32_t) const;
};

template <class Addr>
class IPNetwork;

template <class Addr>
class IPInterface : protected Addr
{
protected:
    int prefix; // number of bits

public:
    IPInterface() = default;
    IPInterface(const IPInterface&) = default;
    IPInterface(const Addr&, int);
    IPInterface(const std::string&, int);
    IPInterface(uint32_t, int);
    IPInterface(const std::string&);

    std::string to_string() const;
    int prefix_length() const;
    Addr addr() const;
    IPNetwork<Addr> network() const;
    bool operator==(const IPInterface<Addr>&) const;
    bool operator!=(const IPInterface<Addr>&) const;
};

template <class Addr>
class IPRange;

template <class Addr>
class IPNetwork : public IPInterface<Addr>
{
private:
    bool is_network() const; // check if it's the network address

public:
    IPNetwork() = default;
    IPNetwork(const IPNetwork&) = default;
    IPNetwork(const Addr&, int);
    IPNetwork(const std::string&, int);
    IPNetwork(uint32_t, int);
    IPNetwork(const std::string&);
    IPNetwork(const IPRange<Addr>&);

    Addr network_addr() const;
    Addr broadcast_addr() const;
    bool contains(const Addr&) const;
    IPRange<Addr> range() const;
};

template <class Addr>
class IPRange
{
private:
    Addr lb;
    Addr ub;

public:
    IPRange() = default;
    IPRange(const IPRange&) = default;
    IPRange(const Addr& lb, const Addr& ub);
    IPRange(const IPNetwork<Addr>&);

    std::string to_string() const;
    size_t size() const;
    size_t length() const;
    void set_lb(const Addr&);
    void set_ub(const Addr&);
    Addr get_lb() const;
    Addr get_ub() const;
    bool contains(const Addr&) const;
    bool is_network() const;
    IPNetwork<Addr> network() const;
};

////////////////////////////////////////////////////////////////////////////////

template <class Addr>
IPInterface<Addr>::IPInterface(const Addr& addr, int prefix)
    : Addr(addr), prefix(prefix)
{
}

template <class Addr>
IPInterface<Addr>::IPInterface(const std::string& ips, int prefix)
    : Addr(ips), prefix(prefix)
{
}

template <class Addr>
IPInterface<Addr>::IPInterface(uint32_t ip, int prefix)
    : Addr(ip), prefix(prefix)
{
}

template <class Addr>
IPInterface<Addr>::IPInterface(const std::string& cidr)
{
    int parsed, r, oct[4];
    unsigned int prefix_len;

    r = sscanf(cidr.c_str(), "%d.%d.%d.%d/%u%n", oct, oct + 1, oct + 2, oct + 3,
               &prefix_len, &parsed);
    if (r != 5 || parsed != (int)cidr.size()) {
        Logger::get_instance().err("Failed to parse IP: " + cidr);
    }
    Addr::value = 0;
    for (int i = 0; i < 4; ++i) {
        if (oct[i] < 0 || oct[i] > 255) {
            Logger::get_instance().err("Invalid IP octet: " +
                                       std::to_string(oct[i]));
        }
        Addr::value = (Addr::value << 8) + oct[i];
    }
    if (prefix_len > Addr::length()) {
        Logger::get_instance().err("Invalid prefix length: " +
                                   std::to_string(prefix_len));
    }
    prefix = prefix_len;
}

template <class Addr>
std::string IPInterface<Addr>::to_string() const
{
    return Addr::to_string() + "/" + std::to_string(prefix);
}

template <class Addr>
int IPInterface<Addr>::prefix_length() const
{
    return prefix;
}

template <class Addr>
Addr IPInterface<Addr>::addr() const
{
    return Addr(Addr::value);
}

template <class Addr>
IPNetwork<Addr> IPInterface<Addr>::network() const
{
    uint32_t mask = (1 << (Addr::length() - prefix)) - 1;
    return IPNetwork<Addr>(Addr::value & (~mask), prefix);
}

template <class Addr>
bool IPInterface<Addr>::operator==(const IPInterface<Addr>& rhs) const
{
    if (Addr::value == rhs.value && prefix == rhs.prefix) {
        return true;
    } else {
        return false;
    }
}

template <class Addr>
bool IPInterface<Addr>::operator!=(const IPInterface<Addr>& rhs) const
{
    if (Addr::value == rhs.value && prefix == rhs.prefix) {
        return false;
    } else {
        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class Addr>
bool IPNetwork<Addr>::is_network() const
{
    uint32_t mask = (1 << (IPInterface<Addr>::length()
                           - IPInterface<Addr>::prefix)) - 1;
    if ((IPInterface<Addr>::value & mask) == 0) {
        return true;
    } else {
        return false;
    }
}

template <class Addr>
IPNetwork<Addr>::IPNetwork(const Addr& addr, int prefix)
    : IPInterface<Addr>(addr, prefix)
{
    if (!is_network()) {
        Logger::get_instance().err("Invalid network: " +
                                   IPInterface<Addr>::to_string());
    }
}

template <class Addr>
IPNetwork<Addr>::IPNetwork(const std::string& ips, int prefix)
    : IPInterface<Addr>(ips, prefix)
{
    if (!is_network()) {
        Logger::get_instance().err("Invalid network: " +
                                   IPInterface<Addr>::to_string());
    }
}

template <class Addr>
IPNetwork<Addr>::IPNetwork(uint32_t ip, int prefix)
    : IPInterface<Addr>(ip, prefix)
{
    if (!is_network()) {
        Logger::get_instance().err("Invalid network: " +
                                   IPInterface<Addr>::to_string());
    }
}

template <class Addr>
IPNetwork<Addr>::IPNetwork(const std::string& cidr)
    : IPInterface<Addr>(cidr)
{
    if (!is_network()) {
        Logger::get_instance().err("Invalid network: " +
                                   IPInterface<Addr>::to_string());
    }
}

template <class Addr>
IPNetwork<Addr>::IPNetwork(const IPRange<Addr>& range)
{
    uint32_t size = range.size();

    if ((size & (size - 1)) != 0 || (range.get_lb() & (size - 1)) != 0) {
        Logger::get_instance().err("Invalid network: " + range.to_string());
    }

    IPInterface<Addr>::value = range.get_lb().get_value();
    IPInterface<Addr>::prefix = IPInterface<Addr>::length() - (int)log2(size);
}

template <class Addr>
Addr IPNetwork<Addr>::network_addr() const
{
    return IPInterface<Addr>::addr();
}

template <class Addr>
Addr IPNetwork<Addr>::broadcast_addr() const
{
    uint32_t mask = (1 << (IPInterface<Addr>::length()
                           - IPInterface<Addr>::prefix)) - 1;
    return Addr(IPInterface<Addr>::value | mask);
}

template <class Addr>
bool IPNetwork<Addr>::contains(const Addr& addr) const
{
    if (addr >= network_addr() && addr <= broadcast_addr()) {
        return true;
    } else {
        return false;
    }
}

template <class Addr>
IPRange<Addr> IPNetwork<Addr>::range() const
{
    return IPRange<Addr>(*this);
}

////////////////////////////////////////////////////////////////////////////////

template <class Addr>
IPRange<Addr>::IPRange(const Addr& lb, const Addr& ub): lb(lb), ub(ub)
{
    if (lb > ub) {
        Logger::get_instance().err("Invalid IP range: " + to_string());
    }
}

template <class Addr>
IPRange<Addr>::IPRange(const IPNetwork<Addr>& net)
    : lb(net.network_addr()), ub(net.broadcast_addr())
{
    if (lb > ub) {
        Logger::get_instance().err("Invalid IP range: " + to_string());
    }
}

template <class Addr>
std::string IPRange<Addr>::to_string() const
{
    return "[" + lb.to_string() + ", " + ub.to_string() + "]";
}

template <class Addr>
size_t IPRange<Addr>::size() const
{
    return ub - lb + 1;
}

template <class Addr>
size_t IPRange<Addr>::length() const
{
    return size();
}

template <class Addr>
void IPRange<Addr>::set_lb(const Addr& addr)
{
    lb = addr;
}

template <class Addr>
void IPRange<Addr>::set_ub(const Addr& addr)
{
    ub = addr;
}

template <class Addr>
Addr IPRange<Addr>::get_lb() const
{
    return lb;
}

template <class Addr>
Addr IPRange<Addr>::get_ub() const
{
    return ub;
}

template <class Addr>
bool IPRange<Addr>::contains(const Addr& addr) const
{
    if (addr >= lb && addr <= ub) {
        return true;
    } else {
        return false;
    }
}

template <class Addr>
bool IPRange<Addr>::is_network() const
{
    uint32_t n = size();

    if ((n & (n - 1)) != 0 || (lb & (n - 1)) != 0) {
        return false;
    } else {
        return true;
    }
}

template <class Addr>
IPNetwork<Addr> IPRange<Addr>::network() const
{
    return IPNetwork<Addr>(*this);
}
