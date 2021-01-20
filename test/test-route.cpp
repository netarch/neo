#include <catch.hpp>
#include <string>

#include "route.hpp"
#include "lib/fs.hpp"

TEST_CASE("route")
{
    std::shared_ptr<Route> route;

    SECTION("missing network") {
        std::string content =
            "[route]\n"
            "next_hop = \"1.2.3.4\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto route_config = config->get_table("route");
        REQUIRE(route_config);
        CHECK_THROWS_WITH(route = std::make_shared<Route>(route_config),
                          "Missing network");
    }

    SECTION("missing next hop") {
        std::string content =
            "[route]\n"
            "network = \"10.0.0.0/8\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto route_config = config->get_table("route");
        REQUIRE(route_config);
        CHECK_THROWS_WITH(route = std::make_shared<Route>(route_config),
                          "Missing next hop IP address or interface");
    }

    SECTION("invalid administrative distance") {
        std::string content =
            "[route]\n"
            "network = \"10.0.0.0/8\"\n"
            "next_hop = \"1.2.3.4\"\n"
            "adm_dist = 256\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto route_config = config->get_table("route");
        REQUIRE(route_config);
        CHECK_THROWS_WITH(route = std::make_shared<Route>(route_config),
                          "Invalid administrative distance: 256");
    }

    SECTION("static route") {
        std::string content =
            "[route]\n"
            "network = \"10.0.0.0/8\"\n"
            "next_hop = \"1.2.3.4\"\n"
            "adm_dist = 1\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto route_config = config->get_table("route");
        REQUIRE(route_config);
        REQUIRE_NOTHROW(route = std::make_shared<Route>(route_config));
        CHECK(route->to_string() == "10.0.0.0/8 --> 1.2.3.4");
        CHECK(route->get_network() == "10.0.0.0/8");
        CHECK(route->get_next_hop() == "1.2.3.4");
        CHECK(route->get_adm_dist() == 1);
        CHECK(route->get_intf().empty());
    }

    SECTION("relational operations") {
        Route route1(IPNetwork<IPv4Address>("10.0.0.0/8"),
                     IPv4Address("1.2.3.4"));
        Route route2("192.168.0.0/16", "172.16.1.1");

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
}
