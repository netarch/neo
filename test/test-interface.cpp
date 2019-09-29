#include <catch2/catch.hpp>
#include <string>

#include "interface.hpp"
#include "lib/fs.hpp"

TEST_CASE("interface")
{
    Interface *intf = nullptr;

    SECTION("missing interface name") {
        std::string content =
            "[interface]\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto intf_config = config->get_table("interface");
        REQUIRE(intf_config);
        CHECK_THROWS_WITH(intf = new Interface(intf_config),
                          "Missing interface name");
    }

    SECTION("L3 interface") {
        std::string content =
            "[interface]\n"
            "name = \"eth0\"\n"
            "ipv4 = \"1.2.3.4/8\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto intf_config = config->get_table("interface");
        REQUIRE(intf_config);
        REQUIRE_NOTHROW(intf = new Interface(intf_config));
        CHECK(intf->to_string() == "eth0");
        CHECK(intf->get_name() == "eth0");
        CHECK(intf->addr() == "1.2.3.4");
        CHECK(intf->prefix_length() == 8);
        CHECK(intf->network() == "1.0.0.0/8");
        CHECK(intf->is_l2() == false);
    }

    SECTION("L2 interface") {
        std::string content =
            "[interface]\n"
            "name = \"br0\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto intf_config = config->get_table("interface");
        REQUIRE(intf_config);
        REQUIRE_NOTHROW(intf = new Interface(intf_config));
        CHECK(intf->to_string() == "br0");
        CHECK(intf->get_name() == "br0");
        CHECK_THROWS_WITH(intf->addr(), "Switchport: br0");
        CHECK_THROWS_WITH(intf->prefix_length(), "Switchport: br0");
        CHECK_THROWS_WITH(intf->network(), "Switchport: br0");
        CHECK(intf->is_l2() == true);
    }

    delete intf;
}
