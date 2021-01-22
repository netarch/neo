#include <string>
#include <catch2/catch.hpp>

#include "config.hpp"
#include "network.hpp"
#include "node.hpp"

TEST_CASE("node")
{
    Network network;
    const std::string input_filename = "networks/node.toml";
    REQUIRE_NOTHROW(Config::start_parsing(input_filename));
    REQUIRE_NOTHROW(Config::parse_network(&network, input_filename));
    REQUIRE_NOTHROW(Config::finish_parsing(input_filename));

    Node *r0, *r1;
    REQUIRE_NOTHROW(r0 = network.get_nodes().at("r0"));
    REQUIRE_NOTHROW(r1 = network.get_nodes().at("r1"));
    REQUIRE(r0);
    REQUIRE(r1);

    SECTION("basic information access (r0)") {
        CHECK_NOTHROW(r0->init());
        CHECK(r0->to_string() == "r0");
        CHECK(r0->get_name() == "r0");
        CHECK_FALSE(r0->has_ip("192.168.1.1"));
        CHECK_FALSE(r0->has_ip("10.0.0.1"));
        CHECK_FALSE(r0->is_l3_only());
        CHECK(r0->get_interface("eth0"));
        CHECK(r0->get_interface("eth1"));
        CHECK_THROWS(r0->get_interface("eth2"));
        CHECK_THROWS(r0->get_interface(IPv4Address("192.168.1.1")));
        CHECK_THROWS(r0->get_interface(IPv4Address("10.0.0.1")));
        CHECK(r0->get_intfs().size() == 2);
        CHECK(r0->get_intfs_l3().size() == 0);
        CHECK(r0->get_intfs_l2().size() == 2);
        CHECK_NOTHROW(r0->get_rib());
    }

    SECTION("basic information access (r1)") {
        CHECK_NOTHROW(r1->init());
        CHECK(r1->to_string() == "r1");
        CHECK(r1->get_name() == "r1");
        CHECK(r1->has_ip("192.168.1.1"));
        CHECK(r1->has_ip("10.0.0.1"));
        CHECK_FALSE(r1->has_ip("1.2.3.4"));
        CHECK(r1->is_l3_only());
        CHECK(r1->get_interface("eth0"));
        CHECK(r1->get_interface("eth1"));
        CHECK_THROWS(r1->get_interface("eth2"));
        CHECK(r1->get_interface(IPv4Address("192.168.1.1")));
        CHECK(r1->get_interface(IPv4Address("10.0.0.1")));
        CHECK_THROWS(r1->get_interface(IPv4Address("1.2.3.4")));
        CHECK(r1->get_intfs().size() == 2);
        CHECK(r1->get_intfs_l3().size() == 2);
        CHECK(r1->get_intfs_l2().size() == 0);
        CHECK_NOTHROW(r1->get_rib());
    }

    SECTION("peer check") {
        std::pair<Node *, Interface *> r0eth0_peer = r0->get_peer("eth0");
        std::pair<Node *, Interface *> r0eth1_peer = r0->get_peer("eth1");
        std::pair<Node *, Interface *> r1eth0_peer = r1->get_peer("eth0");
        std::pair<Node *, Interface *> r1eth1_peer = r1->get_peer("eth1");
        CHECK(r0eth0_peer.first == r1);
        CHECK(r0eth0_peer.second == r1->get_interface("eth0"));
        CHECK(r0eth1_peer.first == r1);
        CHECK(r0eth1_peer.second == r1->get_interface("eth1"));
        CHECK(r1eth0_peer.first == r0);
        CHECK(r1eth0_peer.second == r0->get_interface("eth0"));
        CHECK(r1eth1_peer.first == r0);
        CHECK(r1eth1_peer.second == r0->get_interface("eth1"));
    }

    SECTION("L2_LAN check") {
        L2_LAN *lan = r0->get_l2lan(r0->get_interface("eth0"));
        REQUIRE(lan);
        CHECK(lan == r0->get_l2lan(r0->get_interface("eth1")));
        CHECK(lan == r1->get_l2lan(r1->get_interface("eth0")));
        CHECK(lan == r1->get_l2lan(r1->get_interface("eth1")));
    }

    SECTION("next hops") {
        CHECK(r0->get_ipnhs("10.0.0.1").empty());
        CHECK(r0->get_ipnhs("8.8.8.8").empty());
        CHECK(r0->get_ipnh("eth0", "8.8.8.8") ==
              FIB_IPNH(r1, r1->get_interface("eth0"), r1, r1->get_interface("eth0")));
        CHECK(r0->get_ipnh("eth1", "8.8.8.8") ==
              FIB_IPNH(r1, r1->get_interface("eth1"), r1, r1->get_interface("eth1")));

        REQUIRE(r1->get_ipnhs("192.168.1.1").size() == 1);
        CHECK(*(r1->get_ipnhs("192.168.1.1").begin()) == FIB_IPNH(r1, nullptr, r1, nullptr));
        CHECK(r1->get_ipnhs("192.168.1.2").empty());
        REQUIRE(r1->get_ipnhs("10.0.0.1").size() == 1);
        CHECK(*(r1->get_ipnhs("10.0.0.1").begin()) == FIB_IPNH(r1, nullptr, r1, nullptr));
        CHECK(r1->get_ipnhs("10.0.0.2").empty());
        REQUIRE(r1->get_ipnhs("10.0.1.1").size() == 1);
        CHECK(*(r1->get_ipnhs("10.0.1.1").begin()) == FIB_IPNH(r1, nullptr, r1, nullptr));
        CHECK(r1->get_ipnhs("8.8.8.8").empty());
        CHECK(r1->get_ipnh("eth0", "8.8.8.8") == FIB_IPNH(nullptr, nullptr, nullptr, nullptr));
        CHECK(r1->get_ipnh("eth0", "10.0.0.1") ==
              FIB_IPNH(r1, r1->get_interface("eth1"), r0, r0->get_interface("eth0")));
        CHECK(r1->get_ipnh("eth1", "8.8.8.8") == FIB_IPNH(nullptr, nullptr, nullptr, nullptr));
        CHECK(r1->get_ipnh("eth1", "192.168.1.1") ==
              FIB_IPNH(r1, r1->get_interface("eth0"), r0, r0->get_interface("eth1")));
    }
}
