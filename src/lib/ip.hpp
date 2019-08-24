#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <functional>

#include "lib/logger.hpp"

class IPv4Address
{
protected:
    uint32_t value;

public:
    IPv4Address() = default;
    IPv4Address(const IPv4Address&) = default;
    IPv4Address(IPv4Address&&) = default;
    IPv4Address(const std::string&);
    IPv4Address(const char *);
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
    IPv4Address& operator=(const IPv4Address&) = default;
    IPv4Address& operator=(IPv4Address&&) = default;
    IPv4Address& operator=(const std::string&);
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
    IPInterface(IPInterface&&) = default;
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
    bool operator==(const std::string&) const;
    bool operator!=(const std::string&) const;
    IPInterface<Addr>& operator=(const IPInterface<Addr>&) = default;
    IPInterface<Addr>& operator=(IPInterface<Addr>&&) = default;
    IPInterface<Addr>& operator=(const std::string&);
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
    IPNetwork(IPNetwork&&) = default;
    IPNetwork(const Addr&, int);
    IPNetwork(const std::string&, int);
    IPNetwork(uint32_t, int);
    IPNetwork(const std::string&);
    IPNetwork(const IPRange<Addr>&);

    Addr network_addr() const;
    Addr broadcast_addr() const;
    bool contains(const Addr&) const;
    IPRange<Addr> range() const;
    bool operator==(const IPNetwork<Addr>&) const;
    bool operator!=(const IPNetwork<Addr>&) const;
    bool operator==(const std::string&) const;
    bool operator!=(const std::string&) const;
    IPNetwork<Addr>& operator=(const IPNetwork<Addr>&) = default;
    IPNetwork<Addr>& operator=(IPNetwork<Addr>&&) = default;
    IPNetwork<Addr>& operator=(const std::string&);
};

template <class Addr>
class IPRange
{
protected:
    Addr lb;
    Addr ub;

public:
    IPRange() = default;
    IPRange(const IPRange&) = default;
    IPRange(IPRange&&) = default;
    IPRange(const Addr& lb, const Addr& ub);
    IPRange(const std::string& lb, const std::string& ub);
    IPRange(const IPNetwork<Addr>&);
    IPRange(const std::string&);

    std::string to_string() const;
    size_t size() const;
    size_t length() const;
    void set_lb(const Addr&);
    void set_ub(const Addr&);
    void set_lb(const std::string&);
    void set_ub(const std::string&);
    Addr get_lb() const;
    Addr get_ub() const;
    bool contains(const Addr&) const;
    bool contains(const IPNetwork<Addr>&) const;
    bool contains(const IPRange<Addr>&) const;
    bool is_network() const;
    IPNetwork<Addr> network() const;
    bool operator==(const IPRange<Addr>&) const;
    bool operator!=(const IPRange<Addr>&) const;
    bool operator==(const IPNetwork<Addr>&) const;
    bool operator!=(const IPNetwork<Addr>&) const;
    bool operator==(const std::string&) const;
    bool operator!=(const std::string&) const;
    IPRange<Addr>& operator=(const IPRange<Addr>&) = default;
    IPRange<Addr>& operator=(IPRange<Addr>&&) = default;
    IPRange<Addr>& operator=(const IPNetwork<Addr>&);
    IPRange<Addr>& operator=(const std::string&);
};

////////////////////////////////////////////////////////////////////////////////

template <class Addr>
IPInterface<Addr>::IPInterface(const Addr& addr, int prefix)
    : Addr(addr), prefix(prefix)
{
    if (prefix < 0 || (size_t)prefix > Addr::length()) {
        Logger::get_instance().err("Invalid prefix length: " +
                                   std::to_string(prefix));
    }
}

template <class Addr>
IPInterface<Addr>::IPInterface(const std::string& ips, int prefix)
    : Addr(ips), prefix(prefix)
{
    if (prefix < 0 || (size_t)prefix > Addr::length()) {
        Logger::get_instance().err("Invalid prefix length: " +
                                   std::to_string(prefix));
    }
}

template <class Addr>
IPInterface<Addr>::IPInterface(uint32_t ip, int prefix)
    : Addr(ip), prefix(prefix)
{
    if (prefix < 0 || (size_t)prefix > Addr::length()) {
        Logger::get_instance().err("Invalid prefix length: " +
                                   std::to_string(prefix));
    }
}

template <class Addr>
IPInterface<Addr>::IPInterface(const std::string& cidr)
{
    int r, prefix_len, parsed, oct[4];

    r = sscanf(cidr.c_str(), "%d.%d.%d.%d/%d%n", oct, oct + 1, oct + 2, oct + 3,
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
    prefix = prefix_len;
    if (prefix < 0 || (size_t)prefix > Addr::length()) {
        Logger::get_instance().err("Invalid prefix length: " +
                                   std::to_string(prefix));
    }
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
    return (Addr::value == rhs.value && prefix == rhs.prefix);
}

template <class Addr>
bool IPInterface<Addr>::operator!=(const IPInterface<Addr>& rhs) const
{
    return (Addr::value != rhs.value || prefix != rhs.prefix);
}

template <class Addr>
bool IPInterface<Addr>::operator==(const std::string& rhs) const
{
    return *this == IPInterface<Addr>(rhs);
}

template <class Addr>
bool IPInterface<Addr>::operator!=(const std::string& rhs) const
{
    return *this != IPInterface<Addr>(rhs);
}

template <class Addr>
IPInterface<Addr>& IPInterface<Addr>::operator=(const std::string& rhs)
{
    return (*this = IPInterface<Addr>(rhs));
}

////////////////////////////////////////////////////////////////////////////////

template <class Addr>
bool IPNetwork<Addr>::is_network() const
{
    uint32_t mask = (1ULL << (IPInterface<Addr>::length()
                              - IPInterface<Addr>::prefix)) - 1;
    return ((IPInterface<Addr>::value & mask) == 0);
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
    uint32_t mask = (1ULL << (IPInterface<Addr>::length()
                              - IPInterface<Addr>::prefix)) - 1;
    return Addr(IPInterface<Addr>::value | mask);
}

template <class Addr>
bool IPNetwork<Addr>::contains(const Addr& addr) const
{
    return (addr >= network_addr() && addr <= broadcast_addr());
}

template <class Addr>
IPRange<Addr> IPNetwork<Addr>::range() const
{
    return IPRange<Addr>(*this);
}

template <class Addr>
bool IPNetwork<Addr>::operator==(const IPNetwork<Addr>& rhs) const
{
    return IPInterface<Addr>::operator==(rhs);
}

template <class Addr>
bool IPNetwork<Addr>::operator!=(const IPNetwork<Addr>& rhs) const
{
    return IPInterface<Addr>::operator!=(rhs);
}

template <class Addr>
bool IPNetwork<Addr>::operator==(const std::string& rhs) const
{
    return IPInterface<Addr>::operator==(rhs);
}

template <class Addr>
bool IPNetwork<Addr>::operator!=(const std::string& rhs) const
{
    return IPInterface<Addr>::operator!=(rhs);
}

template <class Addr>
IPNetwork<Addr>& IPNetwork<Addr>::operator=(const std::string& rhs)
{
    IPInterface<Addr>::operator=(rhs);
    return *this;
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
IPRange<Addr>::IPRange(const std::string& lbs, const std::string& ubs)
    : lb(lbs), ub(ubs)
{
    if (lb > ub) {
        Logger::get_instance().err("Invalid IP range: " + to_string());
    }
}

template <class Addr>
IPRange<Addr>::IPRange(const IPNetwork<Addr>& net)
    : lb(net.network_addr()), ub(net.broadcast_addr())
{
}

template <class Addr>
IPRange<Addr>::IPRange(const std::string& net)
    : IPRange(IPNetwork<Addr>(net))
{
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
void IPRange<Addr>::set_lb(const std::string& addr)
{
    lb = addr;
}

template <class Addr>
void IPRange<Addr>::set_ub(const std::string& addr)
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
    return (addr >= lb && addr <= ub);
}

template <class Addr>
bool IPRange<Addr>::contains(const IPNetwork<Addr>& net) const
{
    return (net.network_addr() >= lb && net.broadcast_addr() <= ub);
}

template <class Addr>
bool IPRange<Addr>::contains(const IPRange<Addr>& range) const
{
    return (range.lb >= lb && range.ub <= ub);
}

template <class Addr>
bool IPRange<Addr>::is_network() const
{
    uint32_t n = size();
    return ((n & (n - 1)) == 0 && (lb & (n - 1)) == 0);
}

template <class Addr>
IPNetwork<Addr> IPRange<Addr>::network() const
{
    return IPNetwork<Addr>(*this);
}

template <class Addr>
bool IPRange<Addr>::operator==(const IPRange<Addr>& rhs) const
{
    return (lb == rhs.lb && ub == rhs.ub);
}

template <class Addr>
bool IPRange<Addr>::operator!=(const IPRange<Addr>& rhs) const
{
    return (lb != rhs.lb || ub != rhs.ub);
}

template <class Addr>
bool IPRange<Addr>::operator==(const IPNetwork<Addr>& rhs) const
{
    return (lb == rhs.network_addr() && ub == rhs.broadcast_addr());
}

template <class Addr>
bool IPRange<Addr>::operator!=(const IPNetwork<Addr>& rhs) const
{
    return (lb != rhs.network_addr() || ub != rhs.broadcast_addr());
}

template <class Addr>
bool IPRange<Addr>::operator==(const std::string& rhs) const
{
    return *this == IPRange<Addr>(rhs);
}

template <class Addr>
bool IPRange<Addr>::operator!=(const std::string& rhs) const
{
    return *this != IPRange<Addr>(rhs);
}

template <class Addr>
IPRange<Addr>& IPRange<Addr>::operator=(const IPNetwork<Addr>& rhs)
{
    return (*this = IPRange<Addr>(rhs));
}

template <class Addr>
IPRange<Addr>& IPRange<Addr>::operator=(const std::string& rhs)
{
    return (*this = IPRange<Addr>(rhs));
}

////////////////////////////////////////////////////////////////////////////////

namespace std
{

template <>
struct hash<IPv4Address> {
    size_t operator()(const IPv4Address& addr) const
    {
        return hash<uint32_t>()(addr.get_value());
    }
};

} // namespace std
