#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "configparser.hpp"
#include "interface.hpp"
#include "network.hpp"
#include "node.hpp"
#include "plankton.hpp"

using namespace std;

extern string test_data_dir;

TEST_CASE("interface") {
    auto &plankton = Plankton::get();
    plankton.reset();
    const string inputfn = test_data_dir + "/interface.toml";
    REQUIRE_NOTHROW(ConfigParser().parse(inputfn, plankton));
    const auto &network = plankton.network();

    Node *node;
    REQUIRE_NOTHROW(node = network.nodes().at("r0"));
    REQUIRE(node);

    SECTION("L3 interface") {
        Interface *interface;
        REQUIRE_NOTHROW(interface = node->get_interface("eth0"));
        REQUIRE(interface);
        CHECK(interface->to_string() == "eth0");
        CHECK(interface->get_name() == "eth0");
        CHECK(interface->prefix_length() == 24);
        CHECK(interface->addr() == "192.168.1.11");
        CHECK(interface->mask() == "255.255.255.0");
        CHECK(interface->network() == "192.168.1.0/24");
        CHECK(interface->is_l2() == false);

        REQUIRE_NOTHROW(interface = node->get_interface("eth1"));
        REQUIRE(interface);
        CHECK(interface->to_string() == "eth1");
        CHECK(interface->get_name() == "eth1");
        CHECK(interface->prefix_length() == 10);
        CHECK(interface->addr() == "172.16.1.2");
        CHECK(interface->mask() == "255.192.0.0");
        CHECK(interface->network() == "172.0.0.0/10");
        CHECK(interface->is_l2() == false);
    }

    SECTION("L2 interface") {
        Interface *interface;
        REQUIRE_NOTHROW(interface = node->get_interface("br0"));
        REQUIRE(interface);
        CHECK(interface->to_string() == "br0");
        CHECK(interface->get_name() == "br0");
        CHECK_THROWS_WITH(interface->prefix_length(), "Switchport: br0");
        CHECK_THROWS_WITH(interface->addr(), "Switchport: br0");
        CHECK_THROWS_WITH(interface->mask(), "Switchport: br0");
        CHECK_THROWS_WITH(interface->network(), "Switchport: br0");
        CHECK(interface->is_l2());
    }
}
