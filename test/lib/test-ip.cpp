#include <catch2/catch.hpp>
#include <memory>
#include <utility>

#include "lib/ip.hpp"

TEST_CASE("ipv4address")
{
    IPv4Address addr("192.168.1.1");
    IPv4Address addr2(addr + 1);

    SECTION("constructors") {
        std::shared_ptr<IPv4Address> ip;

        REQUIRE_NOTHROW(ip = std::make_shared<IPv4Address>());
        CHECK(*ip == "0.0.0.0");
        REQUIRE_NOTHROW(ip = std::make_shared<IPv4Address>(addr));
        CHECK(*ip == "192.168.1.1");
        REQUIRE_NOTHROW(ip = std::make_shared<IPv4Address>(std::move(addr)));
        CHECK(*ip == "192.168.1.1");
        REQUIRE_NOTHROW(ip = std::make_shared<IPv4Address>("192.168.1.1"));
        CHECK(*ip == "192.168.1.1");
        CHECK_THROWS_WITH(std::make_shared<IPv4Address>
                          ("invalid format"),
                          "Failed to parse IP: invalid format");
        CHECK_THROWS_WITH(std::make_shared<IPv4Address>
                          ("10.0.0.256"),
                          "Invalid IP octet: 256");
        REQUIRE_NOTHROW(ip = std::make_shared<IPv4Address>(3232235777U));
        CHECK(*ip == "192.168.1.1");
    }

    SECTION("basic information access") {
        CHECK("192.168.1.1" == addr.to_string());
        CHECK("192.168.1.2" == addr2.to_string());
        CHECK(32U == addr.length());
        CHECK(32U == addr2.length());
        CHECK(3232235777U == addr.get_value());
        CHECK(3232235778U == addr2.get_value());
    }

    SECTION("relational operations") {
        CHECK(addr < addr2);
        CHECK_FALSE(addr2 < addr);
        CHECK(addr <= addr);
        CHECK(addr <= addr2);
        CHECK_FALSE(addr2 <= addr);
        CHECK(addr2 > addr);
        CHECK_FALSE(addr > addr2);
        CHECK(addr >= addr);
        CHECK(addr2 >= addr);
        CHECK_FALSE(addr >= addr2);
        CHECK(addr == addr);
        CHECK(addr2 == addr2);
        CHECK_FALSE(addr == addr2);
        CHECK_FALSE(addr2 == addr);
        CHECK(addr != addr2);
        CHECK_FALSE(addr != addr);
        CHECK_FALSE(addr2 != addr2);

        CHECK(addr < "192.169.0.0");
        CHECK_FALSE(addr < "192.168.1.1");
        CHECK_FALSE(addr < "192.168.0.1");
        CHECK(addr <= "192.169.0.0");
        CHECK(addr <= "192.168.1.1");
        CHECK_FALSE(addr <= "192.168.0.1");
        CHECK(addr > "192.168.0.1");
        CHECK_FALSE(addr > "192.168.1.1");
        CHECK_FALSE(addr > "192.169.0.0");
        CHECK(addr >= "192.168.0.1");
        CHECK(addr >= "192.168.1.1");
        CHECK_FALSE(addr >= "192.169.0.0");
        CHECK(addr == "192.168.1.1");
        CHECK(addr2 == "192.168.1.2");
        CHECK(addr != "10.0.0.1");
        CHECK(addr2 != "10.0.0.2");
        CHECK_FALSE(addr == "10.0.0.1");
        CHECK_FALSE(addr2 == "10.0.0.2");
        CHECK_FALSE(addr != "192.168.1.1");
        CHECK_FALSE(addr2 != "192.168.1.2");
    }

    IPv4Address ip0("0.0.0.0");
    IPv4Address ip1("0.0.0.1");
    IPv4Address ip2("0.0.0.2");

    SECTION("arithmetic operations") {
        CHECK(addr + ip1 == "192.168.1.2");
        CHECK(addr + ip2 == "192.168.1.3");
        CHECK(addr + 100 == "192.168.1.101");
        CHECK(addr + 256 == "192.168.2.1");
        CHECK(addr2 - addr == 1);
        CHECK(addr - addr2 == -1);
        CHECK(addr - 257 == IPv4Address("192.168.0.0").get_value());
        CHECK(addr2 - addr2.get_value() == 0);
        CHECK((addr & addr2) == "192.168.1.0");
        CHECK((addr & "192.0.3.0") == "192.0.1.0");
        CHECK((addr & 1) == 1);
        CHECK((addr2 & 1) == 0);
        CHECK((addr | addr2) == "192.168.1.3");
        CHECK((addr | "192.0.3.0") == "192.168.3.1");
        CHECK((addr | 1) == addr.get_value());
        CHECK((addr2 | 1) == addr2.get_value() + 1);
    }

    SECTION("assignment operations") {
        IPv4Address ip;
        REQUIRE((ip = addr) == addr);
        REQUIRE((ip = IPv4Address("192.168.1.1")) == addr);
        REQUIRE((ip = "192.168.1.1") == addr);
        REQUIRE((ip += ip1) == addr2);
        REQUIRE((ip -= ip1) == addr);
        REQUIRE((ip +=   1) == addr2);
        REQUIRE((ip -=   1) == addr);
        REQUIRE((ip += 256) == "192.168.2.1");
        REQUIRE((ip -= 256) == "192.168.1.1");
        REQUIRE((ip &= addr2) == "192.168.1.0");
        REQUIRE((ip &= "0.0.0.1") == ip0);
        REQUIRE((ip |= addr) == addr);
        REQUIRE((ip |= "0.0.0.2") == "192.168.1.3");
    }
}

TEST_CASE("ipinterface")
{
    IPv4Address addr("192.168.1.1");
    IPInterface<IPv4Address> intf(addr, 24);

    SECTION("constructors") {
        std::shared_ptr<IPInterface<IPv4Address> > it;

        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >());
        CHECK(it->addr().get_value() == 0);
        CHECK(it->prefix_length() == 0);
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >(intf));
        CHECK(it->addr() == addr);
        CHECK(it->prefix_length() == 24);
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >
                             (std::move(intf)));
        CHECK(it->addr() == addr);
        CHECK(it->prefix_length() == 24);
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >
                             (addr, 17));
        CHECK(it->addr() == addr);
        CHECK(it->prefix_length() == 17);
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          (addr, -1),
                          "Invalid prefix length: -1");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          (addr, 33),
                          "Invalid prefix length: 33");
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >
                             ("10.0.0.1", 32));
        CHECK(it->addr() == "10.0.0.1");
        CHECK(it->prefix_length() == 32);
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("10.0.0.1", -1),
                          "Invalid prefix length: -1");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("10.0.0.1", 33),
                          "Invalid prefix length: 33");
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >
                             (1, 0));
        CHECK(it->addr() == "0.0.0.1");
        CHECK(it->prefix_length() == 0);
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          (1, -1),
                          "Invalid prefix length: -1");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          (1, 33),
                          "Invalid prefix length: 33");
        REQUIRE_NOTHROW(it = std::make_shared<IPInterface<IPv4Address> >
                             ("10.0.0.1/16"));
        CHECK(it->addr() == "10.0.0.1");
        CHECK(it->prefix_length() == 16);
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.4.5/6"),
                          "Failed to parse IP: 1.2.3.4.5/6");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.4/5/6"),
                          "Failed to parse IP: 1.2.3.4/5/6");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.-1/32"),
                          "Invalid IP octet: -1");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.256/32"),
                          "Invalid IP octet: 256");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.4/-1"),
                          "Invalid prefix length: -1");
        CHECK_THROWS_WITH(std::make_shared<IPInterface<IPv4Address> >
                          ("1.2.3.4/33"),
                          "Invalid prefix length: 33");
    }

    SECTION("basic information access") {
        CHECK(intf.to_string() == "192.168.1.1/24");
        CHECK(intf.prefix_length() == 24);
        CHECK(intf.addr() == "192.168.1.1");
        CHECK(intf.network() == IPNetwork<IPv4Address>("192.168.1.0/24"));
        CHECK(IPInterface<IPv4Address>("192.168.0.255/24").network()
              == IPNetwork<IPv4Address>("192.168.0.0/24"));
    }

    SECTION("relational operations") {
        CHECK(intf == IPInterface<IPv4Address>("192.168.1.1/24"));
        CHECK(intf != IPInterface<IPv4Address>("192.168.1.1/25"));
        CHECK(intf == "192.168.1.1/24");
        CHECK(intf != "192.168.1.1/25");
        CHECK_FALSE(intf != IPInterface<IPv4Address>("192.168.1.1/24"));
        CHECK_FALSE(intf == IPInterface<IPv4Address>("192.168.1.1/25"));
        CHECK_FALSE(intf != "192.168.1.1/24");
        CHECK_FALSE(intf == "192.168.1.1/25");
    }

    SECTION("assignment operations") {
        IPInterface<IPv4Address> it;
        CHECK((it = intf) == intf);
        CHECK((it = IPInterface<IPv4Address>(addr, 24)) == intf);
        CHECK((it = "0.0.0.0/0") == "0.0.0.0/0");
    }
}

TEST_CASE("ipnetwork")
{
    IPRange<IPv4Address> range("192.168.2.0", "192.168.3.255");
    IPNetwork<IPv4Address> net("192.168.10.0/24");

    SECTION("constructors") {
        std::shared_ptr<IPNetwork<IPv4Address> > n;

        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >());
        CHECK(n->addr().get_value() == 0);
        CHECK(n->prefix_length() == 0);
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >(net));
        CHECK(n->addr() == "192.168.10.0");
        CHECK(n->prefix_length() == 24);
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            (std::move(net)));
        CHECK(n->addr() == "192.168.10.0");
        CHECK(n->prefix_length() == 24);
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            (IPv4Address("10.0.0.0"), 8));
        CHECK(n->addr() == "10.0.0.0");
        CHECK(n->prefix_length() == 8);
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          (IPv4Address("10.0.0.0"), 0),
                          "Invalid network: 10.0.0.0/0");
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            ("11.0.0.0", 8));
        CHECK(n->addr() == "11.0.0.0");
        CHECK(n->prefix_length() == 8);
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          ("11.0.0.0", 0),
                          "Invalid network: 11.0.0.0/0");
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            (167772160, 16));
        CHECK(n->addr() == "10.0.0.0");
        CHECK(n->prefix_length() == 16);
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          (167772160, 0),
                          "Invalid network: 10.0.0.0/0");
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            ("172.16.0.0/16"));
        CHECK(n->addr() == "172.16.0.0");
        CHECK(n->prefix_length() == 16);
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          ("172.16.0.0/0"),
                          "Invalid network: 172.16.0.0/0");
        REQUIRE_NOTHROW(n = std::make_shared<IPNetwork<IPv4Address> >
                            (range));
        CHECK(n->addr() == "192.168.2.0");
        CHECK(n->prefix_length() == 23);
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          (IPRange<IPv4Address>("10.0.0.0", "10.0.0.4")),
                          "Invalid network: [10.0.0.0, 10.0.0.4]");
        CHECK_THROWS_WITH(std::make_shared<IPNetwork<IPv4Address> >
                          (IPRange<IPv4Address>("10.0.0.1", "10.0.0.4")),
                          "Invalid network: [10.0.0.1, 10.0.0.4]");
    }

    SECTION("basic information access") {
        CHECK(net.to_string() == "192.168.10.0/24");
        CHECK(net.prefix_length() == 24);
        CHECK(net.addr() == "192.168.10.0");
        CHECK(net.network() == net);
        CHECK(net.network() == IPNetwork<IPv4Address>("192.168.10.0/24"));

        CHECK(net.network_addr() == "192.168.10.0");
        CHECK(net.broadcast_addr() == "192.168.10.255");
        CHECK(net.contains(IPv4Address("192.168.10.0")));
        CHECK(net.contains(IPv4Address("192.168.10.255")));
        CHECK_FALSE(net.contains(IPv4Address("192.168.9.255")));
        CHECK_FALSE(net.contains(IPv4Address("192.168.11.0")));
        CHECK(net.range() ==
              IPRange<IPv4Address>("192.168.10.0", "192.168.10.255"));
        CHECK(IPNetwork<IPv4Address>("0.0.0.0/0").range() ==
              IPRange<IPv4Address>("0.0.0.0", "255.255.255.255"));
    }

    SECTION("relational and assignment operators") {
        CHECK(net == IPNetwork<IPv4Address>("192.168.10.0/24"));
        CHECK(net == "192.168.10.0/24");
        CHECK(net != IPNetwork<IPv4Address>("192.168.10.0/23"));
        CHECK(net != "192.168.10.0/23");
        CHECK_FALSE(net != IPNetwork<IPv4Address>("192.168.10.0/24"));
        CHECK_FALSE(net != "192.168.10.0/24");
        CHECK_FALSE(net == IPNetwork<IPv4Address>("192.168.10.0/23"));
        CHECK_FALSE(net == "192.168.10.0/23");
        CHECK((net = IPNetwork<IPv4Address>(range)) == "192.168.2.0/23");
        CHECK((net = "192.168.2.0/23") == "192.168.2.0/23");
    }
}

TEST_CASE("iprange")
{
    IPRange<IPv4Address> range("10.1.4.0", "10.1.7.255");

    SECTION("constructors") {
        std::shared_ptr<IPRange<IPv4Address> > r;

        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >(range));
        CHECK(r->get_lb() == "10.1.4.0");
        CHECK(r->get_ub() == "10.1.7.255");
        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >
                            (std::move(range)));
        CHECK(r->get_lb() == "10.1.4.0");
        CHECK(r->get_ub() == "10.1.7.255");
        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >
                            (IPv4Address("10.0.0.1"), IPv4Address("10.0.0.2")));
        CHECK(r->get_lb() == "10.0.0.1");
        CHECK(r->get_ub() == "10.0.0.2");
        CHECK_THROWS_WITH(std::make_shared<IPRange<IPv4Address> >
                          (IPv4Address("10.0.0.1"), IPv4Address("10.0.0.0")),
                          "Invalid IP range: [10.0.0.1, 10.0.0.0]");
        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >
                            ("10.0.0.1", "10.0.0.2"));
        CHECK(r->get_lb() == "10.0.0.1");
        CHECK(r->get_ub() == "10.0.0.2");
        CHECK_THROWS_WITH(std::make_shared<IPRange<IPv4Address> >
                          ("10.0.0.1", "10.0.0.0"),
                          "Invalid IP range: [10.0.0.1, 10.0.0.0]");
        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >
                            (IPNetwork<IPv4Address>("0.0.0.0/0")));
        CHECK(r->get_lb() == "0.0.0.0");
        CHECK(r->get_ub() == "255.255.255.255");
        REQUIRE_NOTHROW(r = std::make_shared<IPRange<IPv4Address> >
                            ("10.0.113.0/25"));
        CHECK(r->get_lb() == "10.0.113.0");
        CHECK(r->get_ub() == "10.0.113.127");
    }

    SECTION("basic information access") {
        CHECK(range.to_string() == "[10.1.4.0, 10.1.7.255]");
        CHECK(range.size() == 1024);
        CHECK(range.length() == 1024);
        range.set_lb("10.2.4.0");
        range.set_ub("10.2.7.255");
        CHECK(range.get_lb() == "10.2.4.0");
        CHECK(range.get_ub() == "10.2.7.255");
        range.set_lb(IPv4Address("10.1.4.0"));
        range.set_ub(IPv4Address("10.1.7.255"));
        REQUIRE(range.to_string() == "[10.1.4.0, 10.1.7.255]");
        CHECK_FALSE(range.contains(IPv4Address("10.1.3.255")));
        CHECK(range.contains(IPv4Address("10.1.4.0")));
        CHECK(range.contains(IPv4Address("10.1.7.255")));
        CHECK_FALSE(range.contains(IPv4Address("10.1.8.0")));
        CHECK(range.contains(IPNetwork<IPv4Address>("10.1.5.0/24")));
        CHECK(range.contains(IPNetwork<IPv4Address>("10.1.4.0/22")));
        CHECK_FALSE(range.contains(IPNetwork<IPv4Address>("10.0.0.0/8")));
        CHECK(range.contains(IPRange<IPv4Address>("10.1.4.0", "10.1.7.254")));
        CHECK_FALSE(range.contains(IPRange<IPv4Address>
                                   ("10.1.4.1", "10.1.8.0")));
        REQUIRE(range.to_string() == "[10.1.4.0, 10.1.7.255]");
        CHECK(range.is_network());
        CHECK_NOTHROW(range.network());
        range.set_lb("10.1.4.0");
        range.set_ub("10.1.8.255");
        REQUIRE(range.to_string() == "[10.1.4.0, 10.1.8.255]");
        CHECK_FALSE(range.is_network());
        CHECK_THROWS_WITH(range.network(),
                          "Invalid network: [10.1.4.0, 10.1.8.255]");
        range.set_lb("10.1.4.1");
        range.set_ub("10.1.8.0");
        REQUIRE(range.to_string() == "[10.1.4.1, 10.1.8.0]");
        CHECK_FALSE(range.is_network());
        CHECK_THROWS_WITH(range.network(),
                          "Invalid network: [10.1.4.1, 10.1.8.0]");
    }

    SECTION("relational and assignment operators") {
        CHECK(range == IPRange<IPv4Address>("10.1.4.0/22"));
        CHECK(range != IPRange<IPv4Address>("10.1.4.0/23"));
        CHECK_FALSE(range == IPRange<IPv4Address>("10.1.4.0/23"));
        CHECK_FALSE(range != IPRange<IPv4Address>("10.1.4.0/22"));
        CHECK(range == IPNetwork<IPv4Address>("10.1.4.0/22"));
        CHECK(range != IPNetwork<IPv4Address>("10.1.4.0/23"));
        CHECK_FALSE(range == IPNetwork<IPv4Address>("10.1.4.0/23"));
        CHECK_FALSE(range != IPNetwork<IPv4Address>("10.1.4.0/22"));
        CHECK(range == "10.1.4.0/22");
        CHECK(range != "10.1.4.0/23");
        CHECK_FALSE(range == "10.1.4.0/23");
        CHECK_FALSE(range != "10.1.4.0/22");
        CHECK((range = IPRange<IPv4Address>("10.0.0.0", "10.0.0.3"))
              == IPRange<IPv4Address>("10.0.0.0/30"));
        CHECK((range = IPNetwork<IPv4Address>(range))
              == IPRange<IPv4Address>("10.0.0.0", "10.0.0.3"));
        CHECK((range = "10.0.0.0/28") == "10.0.0.0/28");
    }
}
