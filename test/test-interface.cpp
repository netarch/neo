#include <catch2/catch.hpp>

#include "interface.hpp"

TEST_CASE("interface")
{
    Interface intf1("eth0", "10.0.0.1/24");
    Interface intf2("virbr0");

    SECTION("constructors") {
        std::shared_ptr<Interface> intf;

        REQUIRE_NOTHROW(intf = std::make_shared<Interface>(intf1));
        CHECK(intf->get_name() == "eth0");
        CHECK(intf->addr() == "10.0.0.1");
        CHECK(intf->prefix_length() == 24);
        CHECK(intf->switching() == false);
        REQUIRE_NOTHROW(intf = std::make_shared<Interface>("br0"));
        CHECK(intf->get_name() == "br0");
        CHECK(intf->switching() == true);
        REQUIRE_NOTHROW(intf = std::make_shared<Interface>("tun0",
                               "1.1.1.1/32"));
        CHECK(intf->get_name() == "tun0");
        CHECK(intf->addr() == "1.1.1.1");
        CHECK(intf->prefix_length() == 32);
        CHECK(intf->switching() == false);
    }

    SECTION("basic information access") {
        CHECK(intf1.to_string() == "eth0");
        CHECK(intf1.get_name() == "eth0");
        CHECK(intf1.addr() == "10.0.0.1");
        CHECK_THROWS_WITH(intf2.addr(), "Switchport: virbr0");
        CHECK(intf1.prefix_length() == 24);
        CHECK_THROWS_WITH(intf2.prefix_length(), "Switchport: virbr0");
        CHECK(intf1.network() == "10.0.0.0/24");
        CHECK_THROWS_WITH(intf2.network(), "Switchport: virbr0");
        CHECK(intf1.switching() == false);
        CHECK(intf2.switching() == true);
    }
}
