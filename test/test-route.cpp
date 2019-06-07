#include <catch2/catch.hpp>
#include <memory>

#include "route.hpp"

TEST_CASE("route")
{
    Route route1("10.0.0.0/8", "1.2.3.4");
    Route route2("192.168.0.0/16", "172.16.1.1");

    SECTION("constructors") {
        std::shared_ptr<Route> r;

        REQUIRE_NOTHROW(r = std::make_shared<Route>(route1));
        CHECK(r->get_network() == "10.0.0.0/8");
        CHECK(r->get_next_hop() == "1.2.3.4");
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                IPNetwork<IPv4Address>("192.168.113.128/25"),
                                IPv4Address("192.168.1.1")));
        CHECK(r->get_network() == "192.168.113.128/25");
        CHECK(r->get_next_hop() == "192.168.1.1");
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                IPNetwork<IPv4Address>("192.168.113.128/25"),
                                IPv4Address("192.168.1.1"), 1));
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                IPNetwork<IPv4Address>("192.168.113.128/25"),
                                IPv4Address("192.168.1.1"), "eth0"));
        CHECK(r->get_network() == "192.168.113.128/25");
        CHECK(r->get_next_hop() == "192.168.1.1");
        CHECK(r->get_ifname() == "eth0");
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                IPNetwork<IPv4Address>("192.168.113.128/25"),
                                IPv4Address("192.168.1.1"), "eth0", 1));
        REQUIRE_NOTHROW(r = std::make_shared<Route>("0.0.0.0/0", "10.1.1.1"));
        CHECK(r->get_network() == "0.0.0.0/0");
        CHECK(r->get_next_hop() == "10.1.1.1");
        REQUIRE_NOTHROW(r = std::make_shared<Route>("0.0.0.0/0", "10.1.1.1",
                            1));
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                "0.0.0.0/0", "10.1.1.1", "eth0"));
        CHECK(r->get_network() == "0.0.0.0/0");
        CHECK(r->get_next_hop() == "10.1.1.1");
        CHECK(r->get_ifname() == "eth0");
        REQUIRE_NOTHROW(r = std::make_shared<Route>(
                                "0.0.0.0/0", "10.1.1.1", "eth0", 1));
        CHECK(r->get_adm_dist() == 1);
    }

    SECTION("basic information access") {
        CHECK(route1.to_string() == "10.0.0.0/8 --> 1.2.3.4");
        CHECK(route2.to_string() == "192.168.0.0/16 --> 172.16.1.1");
        CHECK(route1.get_network() == "10.0.0.0/8");
        CHECK(route2.get_network() == "192.168.0.0/16");
        CHECK(route1.get_next_hop() == "1.2.3.4");
        CHECK(route2.get_next_hop() == "172.16.1.1");
        CHECK(route1.get_ifname().empty());
        CHECK(route2.get_ifname().empty());
        CHECK(route1.get_adm_dist() == 255);
        CHECK(route2.get_adm_dist() == 255);
        REQUIRE_NOTHROW(route1.set_next_hop(IPv4Address("0.0.1.164")));
        CHECK(route1.to_string() == "10.0.0.0/8 --> 0.0.1.164");
        REQUIRE_NOTHROW(route1.set_next_hop("0.0.0.0"));
        CHECK(route1.to_string() == "10.0.0.0/8 --> 0.0.0.0");
        route1.set_ifname("duck");
        CHECK(route1.get_ifname() == "duck");
    }

    SECTION("relational operations") {
        CHECK(route2 < route1);
        CHECK_FALSE(route1 < route2);
        CHECK(route1 < Route("11.0.0.0/8", "0.0.0.1"));
        CHECK(route2 < Route("192.169.0.0/16", "0.0.0.1"));
        CHECK(route2 <= route1);
        CHECK_FALSE(route1 <= route2);
        CHECK(route1 <= route1);
        CHECK(route2 <= route2);
        CHECK(route1 > route2);
        CHECK_FALSE(route2 > route1);
        CHECK(route1 > Route("9.0.0.0/8", "0.0.0.1"));
        CHECK(route2 > Route("192.167.0.0/16", "0.0.0.1"));
        CHECK(route1 >= route2);
        CHECK_FALSE(route2 >= route1);
        CHECK(route1 >= route1);
        CHECK(route2 >= route2);
        CHECK(route1 == Route("10.0.0.0/8", "0.0.0.1"));
        CHECK_FALSE(route1 == Route("10.0.0.0/7", "0.0.0.1"));
        CHECK_FALSE(route1 == Route("11.0.0.0/8", "0.0.0.1"));
        CHECK(route1 != Route("10.0.0.0/7", "0.0.0.1"));
        CHECK(route1 != Route("11.0.0.0/8", "0.0.0.1"));
        CHECK_FALSE(route1 != Route("10.0.0.0/8", "0.0.0.1"));
        CHECK(route1.has_same_path(route1));
        CHECK(route1.has_same_path(Route("10.0.0.0/8", "1.2.3.4")));
        CHECK_FALSE(route1.has_same_path(Route("10.0.0.0/8", "1.2.3.3")));
        CHECK_FALSE(route1.has_same_path(route2));
    }

    SECTION("assignment operations") {
        route1 = route2;
        CHECK(route1.to_string() == "192.168.0.0/16 --> 172.16.1.1");
    }
}
