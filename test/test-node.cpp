#include <catch2/catch.hpp>
#include <string>

#include "node.hpp"
#include "network.hpp"
#include "lib/fs.hpp"

TEST_CASE("node")
{
    std::shared_ptr<Node> node;

    SECTION("missing node name") {
        std::string content =
            "[node]\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto node_config = config->get_table("node");
        REQUIRE(node_config);
        CHECK_THROWS_WITH(node = std::make_shared<Node>(node_config),
                          "Missing node name");
    }

    SECTION("duplicate interface name") {
        std::string content =
            "[node]\n"
            "   name = \"r0\"\n"
            "   [[node.interfaces]]\n"
            "   name = \"eth0\"\n"
            "   [[node.interfaces]]\n"
            "   name = \"eth0\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto node_config = config->get_table("node");
        REQUIRE(node_config);
        CHECK_THROWS_WITH(node = std::make_shared<Node>(node_config),
                          "Duplicate interface name: eth0");
    }

    SECTION("duplicate interface IP") {
        std::string content =
            "[node]\n"
            "   name = \"r0\"\n"
            "   [[node.interfaces]]\n"
            "   name = \"eth0\"\n"
            "   ipv4 = \"192.168.1.1/30\"\n"
            "   [[node.interfaces]]\n"
            "   name = \"eth1\"\n"
            "   ipv4 = \"192.168.1.1/30\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto node_config = config->get_table("node");
        REQUIRE(node_config);
        CHECK_THROWS_WITH(node = std::make_shared<Node>(node_config),
                          "Duplicate interface IP: 192.168.1.1");
    }

    std::shared_ptr<Network> net;

    SECTION("basic information access") {
        std::string content =
            "[[nodes]]\n"
            "    name = \"r0\"\n"
            "    type = \"generic\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth0\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth1\"\n"
            "    [[nodes.static_routes]]\n"
            "    network = \"0.0.0.0/0\"\n"
            "    next_hop = \"1.2.3.4\"\n"
            "    [[nodes.installed_routes]]\n"
            "    network = \"10.0.0.0/16\"\n"
            "    next_hop = \"192.168.1.1\"\n"
            "[[nodes]]\n"
            "    name = \"r1\"\n"
            "    type = \"generic\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth0\"\n"
            "    ipv4 = \"192.168.1.1/30\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth1\"\n"
            "    ipv4 = \"10.0.0.1/24\"\n"
            "    [[nodes.static_routes]]\n"
            "    network = \"0.0.0.0/0\"\n"
            "    next_hop = \"1.2.3.4\"\n"
            "    [[nodes.installed_routes]]\n"
            "    network = \"10.0.0.0/16\"\n"
            "    next_hop = \"192.168.1.1\"\n"
            "[[links]]\n"
            "    node1 = \"r0\"\n"
            "    intf1 = \"eth0\"\n"
            "    node2 = \"r1\"\n"
            "    intf2 = \"eth0\"\n"
            "[[links]]\n"
            "    node1 = \"r0\"\n"
            "    intf1 = \"eth1\"\n"
            "    node2 = \"r1\"\n"
            "    intf2 = \"eth1\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        REQUIRE_NOTHROW(net = std::make_shared<Network>
                              (nodes_config, links_config));

        REQUIRE_NOTHROW(node = net->get_nodes().at("r1"));
        CHECK(node->to_string() == "r1");
        CHECK(node->get_name() == "r1");
        CHECK(node->has_ip("192.168.1.1"));
        CHECK(node->has_ip("10.0.0.1"));
        CHECK_FALSE(node->has_ip("1.2.3.4"));
        CHECK_NOTHROW(node->get_interface("eth0"));
        CHECK_NOTHROW(node->get_interface("eth1"));
        CHECK_THROWS (node->get_interface("eth2"));
        CHECK_NOTHROW(node->get_interface(IPv4Address("192.168.1.1")));
        CHECK_NOTHROW(node->get_interface(IPv4Address("10.0.0.1")));
        CHECK_THROWS (node->get_interface(IPv4Address("1.2.3.4")));

        std::shared_ptr<Node> r0;
        REQUIRE_NOTHROW(r0 = net->get_nodes().at("r0"));
        auto r0_peer = r0->get_peer("eth0");
        CHECK(r0_peer.first == node);
        CHECK(r0_peer.second == node->get_interface("eth0"));
        auto r1_peer = node->get_peer("eth1");
        CHECK(r1_peer.first == r0);
        CHECK(r1_peer.second == r0->get_interface("eth1"));
        CHECK(r0->get_link("eth0") == node->get_link("eth0"));
        CHECK(r0->get_link("eth1") == node->get_link("eth1"));
    }

    SECTION("two peers/links on one interface") {
        std::string content =
            "[[nodes]]\n"
            "    name = \"r0\"\n"
            "    type = \"generic\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth0\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth1\"\n"
            "[[nodes]]\n"
            "    name = \"r1\"\n"
            "    type = \"generic\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth0\"\n"
            "    [[nodes.interfaces]]\n"
            "    name = \"eth1\"\n"
            "[[links]]\n"
            "    node1 = \"r0\"\n"
            "    intf1 = \"eth0\"\n"
            "    node2 = \"r1\"\n"
            "    intf2 = \"eth0\"\n"
            "[[links]]\n"
            "    node1 = \"r0\"\n"
            "    intf1 = \"eth1\"\n"
            "    node2 = \"r1\"\n"
            "    intf2 = \"eth1\"\n"
            "[[links]]\n"
            "    node1 = \"r0\"\n"
            "    intf1 = \"eth0\"\n"
            "    node2 = \"r1\"\n"
            "    intf2 = \"eth1\"\n"
            ;
        auto config = fs::get_toml_config(content);
        REQUIRE(config);
        auto nodes_config = config->get_table_array("nodes");
        auto links_config = config->get_table_array("links");
        CHECK_THROWS_WITH(net = std::make_shared<Network>
                                (nodes_config, links_config),
                          Catch::StartsWith("Two peers on interface"));
    }
}
