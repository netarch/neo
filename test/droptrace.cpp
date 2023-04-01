#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "droptrace.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"

using namespace std;

extern string test_data_dir;

TEST_CASE("droptrace") {
    auto &plankton = Plankton::get();
    plankton.reset();
    const string inputfn = test_data_dir + "/docker-drop.toml";
    REQUIRE_NOTHROW(ConfigParser().parse(inputfn, plankton));
    const auto &network = plankton.network();
    DockerNode *node;
    REQUIRE_NOTHROW(node = static_cast<DockerNode *>(network.nodes().at("fw")));
    REQUIRE(node);
    Interface *eth0;
    Interface *eth1;
    REQUIRE_NOTHROW(eth0 = node->get_intfs().at("eth0"));
    REQUIRE_NOTHROW(eth1 = node->get_intfs().at("eth1"));
    REQUIRE(eth0);
    REQUIRE(eth1);

    chrono::seconds timeout(1);
    Docker docker(node, /* log_pkts */ false);
    DropTrace &dt = DropTrace::get();

    SECTION("Initialize and terminate the droptrace module") {
        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.start());
        REQUIRE_NOTHROW(dt.stop());
        REQUIRE_NOTHROW(dt.teardown());
        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.teardown());
    }

    SECTION("Uninitialized module") {
        REQUIRE_NOTHROW(dt.teardown());
        REQUIRE_NOTHROW(dt.start());
        REQUIRE_NOTHROW(dt.stop());
        REQUIRE_NOTHROW(dt.start_listening_for(Packet()));
        CHECK(dt.get_drop_ts() == 0);
        REQUIRE_NOTHROW(dt.stop_listening());
    }

    SECTION("Packet drops (sequential)") {
        REQUIRE_NOTHROW(docker.init());
        REQUIRE_NOTHROW(docker.pause());
        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.start()); // Open and load the BPF program

        Packet pkt;
        size_t nwrite;
        uint64_t drop_ts;

        // Ping request packet from node1 to node2
        pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program
        REQUIRE_NOTHROW(dt.start_listening_for(pkt));
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        REQUIRE_NOTHROW(drop_ts = dt.get_drop_ts(timeout));
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Detach the BPF program
        REQUIRE_NOTHROW(dt.stop_listening());

        REQUIRE_NOTHROW(dt.stop()); // Remove the BPF program
        REQUIRE_NOTHROW(dt.teardown());
        REQUIRE_NOTHROW(docker.teardown());
    }
}
