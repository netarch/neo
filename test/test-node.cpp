#include <catch2/catch.hpp>
#include <memory>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "topology.hpp"

TEST_CASE("node constructors")
{
    std::shared_ptr<Node> node;
    Node r1("router1", "generic");

    REQUIRE_NOTHROW(node = std::make_shared<Node>(r1));
    CHECK(node->get_name() == "router1");
    CHECK(node->get_type() == "generic");
    REQUIRE_NOTHROW(node = std::make_shared<Node>("switch1", "generic"));
    CHECK(node->get_name() == "switch1");
    CHECK(node->get_type() == "generic");
    CHECK_THROWS_WITH(node = std::make_shared<Node>("", "blah"),
                      "Unsupported node type: blah");
}

TEST_CASE("node loading configurations")
{
    Topology topology;

    SECTION("success") {
        auto config = cpptoml::parse_file("test-networks/000-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_NOTHROW(topology.load_config(nodes_config, links_config));
    }
    SECTION("route ignored") {
        auto config = cpptoml::parse_file("test-networks/001-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_NOTHROW(topology.load_config(nodes_config, links_config));
    }
    SECTION("key error interface name") {
        auto config = cpptoml::parse_file("test-networks/002-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Key error: name");
    }
    SECTION("duplicate interface name") {
        auto config = cpptoml::parse_file("test-networks/003-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Duplicate interface name: eth0");
    }
    SECTION("duplicate interface ip") {
        auto config = cpptoml::parse_file("test-networks/004-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Duplicate interface IP: 192.168.1.1");
    }
    SECTION("key error static route network") {
        auto config = cpptoml::parse_file("test-networks/005-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Key error: network");
    }
    SECTION("key error static route next hop") {
        auto config = cpptoml::parse_file("test-networks/006-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Key error: next_hop");
    }
    SECTION("duplicate static route") {
        auto config = cpptoml::parse_file("test-networks/007-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Duplicate static route: 0.0.0.0/0 --> 1.2.3.4");
    }
    SECTION("key error installed route network") {
        auto config = cpptoml::parse_file("test-networks/008-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Key error: network");
    }
    SECTION("key error installed route next hop") {
        auto config = cpptoml::parse_file("test-networks/009-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "Key error: next_hop");
    }
    SECTION("RIB entry mismatch") {
        auto config = cpptoml::parse_file("test-networks/00a-test-node.toml");
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(topology.load_config(nodes_config, links_config),
                          "RIB entry mismatch: 10.0.0.0/16 --> 192.168.1.1");
    }
}

TEST_CASE("node test bad peer/link and basic information access")
{
    int i;
    std::shared_ptr<Node> L2node, L3node;
    std::shared_ptr<Link> link1, link2;

    auto config = cpptoml::parse_file("test-networks/000-test-node.toml");
    REQUIRE(config);
    auto nodes_config = config->get_table_array("nodes");
    REQUIRE(nodes_config);
    auto links_config = config->get_table_array("links");
    REQUIRE(links_config);

    i = 0;
    for (auto node_config : *nodes_config) {
        auto name = node_config->get_as<std::string>("name");
        auto type = node_config->get_as<std::string>("type");
        REQUIRE(name);
        REQUIRE(type);
        REQUIRE(*type == "generic");

        std::shared_ptr<Node> node = std::make_shared<Node>(*name, *type);
        node->load_interfaces(node_config->get_table_array("interfaces"));
        node->load_static_routes(node_config->get_table_array("static_routes"));
        node->load_installed_routes(node_config->get_table_array(
                                        "installed_routes"));
        switch (i) {
            case 0:
                L2node = node;
                break;
            case 1:
                L3node = node;
                break;
        }
        ++i;
    }

    i = 0;
    for (auto link_config : *links_config) {
        auto node1_name = link_config->get_as<std::string>("node1");
        auto intf1_name = link_config->get_as<std::string>("intf1");
        auto node2_name = link_config->get_as<std::string>("node2");
        auto intf2_name = link_config->get_as<std::string>("intf2");
        REQUIRE(*node1_name == "L2node");
        REQUIRE(*node2_name == "L3node");

        std::shared_ptr<Node>& node1 = L2node;
        std::shared_ptr<Node>& node2 = L3node;
        const std::shared_ptr<Interface>& intf1 =
            node1->get_interface(*intf1_name);
        const std::shared_ptr<Interface>& intf2 =
            node2->get_interface(*intf2_name);
        std::shared_ptr<Link> link = std::make_shared<Link>
                                     (node1, intf1, node2, intf2);
        switch (i) {
            case 0:
                link1 = link;
            /* fall through */
            case 1:
                link2 = link;
                CHECK_NOTHROW(node1->add_peer(*intf1_name, node2, intf2));
                CHECK_NOTHROW(node2->add_peer(*intf2_name, node1, intf1));
                CHECK_NOTHROW(node1->add_link(*intf1_name, link));
                CHECK_NOTHROW(node2->add_link(*intf2_name, link));
                break;
            case 2:
                CHECK_THROWS_WITH(node1->add_peer(*intf1_name, node2, intf2),
                                  "Two peers on interface: eth0");
                CHECK_THROWS_WITH(node2->add_peer(*intf2_name, node1, intf1),
                                  "Two peers on interface: eth1");
                CHECK_THROWS_WITH(node1->add_link(*intf1_name, link),
                                  "Two links on interface: eth0");
                CHECK_THROWS_WITH(node2->add_link(*intf2_name, link),
                                  "Two links on interface: eth1");
                break;
        }
        ++i;
    }

    SECTION("basic information access") {
        CHECK(L2node->to_string() == "L2node");
        CHECK(L3node->to_string() == "L3node");
        CHECK(L2node->get_name() == "L2node");
        CHECK(L3node->get_name() == "L3node");
        CHECK(L2node->get_type() == "generic");
        CHECK(L3node->get_type() == "generic");
        CHECK(L2node->has_ip("192.168.1.1") == false);
        CHECK(L3node->has_ip("192.168.1.1") == true);
        CHECK(L2node->has_ip("10.0.0.1") == false);
        CHECK(L3node->has_ip("10.0.0.1") == true);
        CHECK(L2node->has_ip("1.2.3.4") == false);
        CHECK(L3node->has_ip("1.2.3.4") == false);
        CHECK_NOTHROW(L2node->get_interface("eth0"));
        CHECK_NOTHROW(L3node->get_interface("eth0"));
        CHECK_NOTHROW(L2node->get_interface("eth1"));
        CHECK_NOTHROW(L3node->get_interface("eth1"));
        CHECK_THROWS (L2node->get_interface("eth2"));
        CHECK_THROWS (L3node->get_interface("eth2"));
        CHECK_THROWS (L2node->get_interface(IPv4Address("192.168.1.1")));
        CHECK_NOTHROW(L3node->get_interface(IPv4Address("192.168.1.1")));
        CHECK_THROWS (L2node->get_interface(IPv4Address("10.0.0.1")));
        CHECK_NOTHROW(L3node->get_interface(IPv4Address("10.0.0.1")));
        CHECK_THROWS (L2node->get_interface(IPv4Address("1.2.3.4")));
        CHECK_THROWS (L3node->get_interface(IPv4Address("1.2.3.4")));
        auto L2node_peer = L2node->get_peer("eth0");
        CHECK(L2node_peer.first == L3node);
        CHECK(L2node_peer.second == L3node->get_interface("eth0"));
        auto L3node_peer = L3node->get_peer("eth1");
        CHECK(L3node_peer.first == L2node);
        CHECK(L3node_peer.second == L2node->get_interface("eth1"));
        CHECK_THROWS(L2node->get_peer("eth2"));
        CHECK_THROWS(L3node->get_peer("eth2"));
        CHECK(L2node->get_link("eth0") == link1);
        CHECK(L3node->get_link("eth0") == link1);
        CHECK(L2node->get_link("eth1") == link2);
        CHECK(L3node->get_link("eth1") == link2);
    }
}
