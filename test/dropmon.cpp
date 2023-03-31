#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "dropmon.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"

using namespace std;

extern string test_data_dir;

TEST_CASE("dropmon") {
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
    DropMon &dm = DropMon::get();

    SECTION("Initialize and terminate the dropmon module") {
        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.start());
        REQUIRE_NOTHROW(dm.stop());
        REQUIRE_NOTHROW(dm.teardown());
        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.teardown());
    }

    SECTION("Uninitialized module") {
        REQUIRE_NOTHROW(dm.teardown());
        REQUIRE_NOTHROW(dm.start());
        REQUIRE_NOTHROW(dm.stop());
        REQUIRE_NOTHROW(dm.start_listening_for(Packet()));
        CHECK(dm.get_drop_ts() == 0);
        REQUIRE_NOTHROW(dm.stop_listening());
    }

    SECTION("Exceptions") {
        // Ping request packet from node1 to node2
        Packet pkt(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                   PS_ICMP_ECHO_REQ);

        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.start());
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        CHECK_THROWS_WITH(dm.start_listening_for(pkt),
                          "dropmon socket is already connected");
        REQUIRE_NOTHROW(dm.stop_listening());
        REQUIRE_NOTHROW(dm.stop_listening());
        REQUIRE_NOTHROW(dm.stop());
    }

    SECTION("Packet drops (sequential)") {
        REQUIRE_NOTHROW(docker.init());
        REQUIRE_NOTHROW(docker.pause());
        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.start());

        // Ping request packet from node1 to node2
        Packet pkt(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                   PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        size_t nwrite;
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        uint64_t drop_ts;
        REQUIRE_NOTHROW(drop_ts = dm.get_drop_ts(timeout));
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        REQUIRE_NOTHROW(dm.stop_listening());

        // Ping request packet from node2 to node1
        pkt = Packet(eth1, "192.168.2.2", "192.168.1.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        REQUIRE_NOTHROW(drop_ts = dm.get_drop_ts(timeout));
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        REQUIRE_NOTHROW(dm.stop_listening());

        // Ping request packet from node1 to fw
        pkt = Packet(eth0, "192.168.1.2", "192.168.1.1", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        REQUIRE_NOTHROW(drop_ts = dm.get_drop_ts(timeout));
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        REQUIRE_NOTHROW(dm.stop_listening());

        // Stop the kernel drop_monitor
        REQUIRE_NOTHROW(dm.stop());
        // Reset and disable the dropmon module
        REQUIRE_NOTHROW(dm.teardown());
        // Reset docker driver object
        REQUIRE_NOTHROW(docker.teardown());
    }

    SECTION("Packet drops (asynchronous)") {
        // Register signal handler to nullify SIGUSR1
        struct sigaction action, *oldaction = nullptr;
        action.sa_handler = [](int) {};
        sigemptyset(&action.sa_mask);
        sigaddset(&action.sa_mask, SIGUSR1);
        action.sa_flags = SA_NOCLDSTOP;
        sigaction(SIGUSR1, &action, oldaction);

        REQUIRE_NOTHROW(docker.init());
        REQUIRE_NOTHROW(docker.pause());
        REQUIRE_NOTHROW(dm.init());
        REQUIRE_NOTHROW(dm.start());

        atomic<bool> stop_dm = false; // loop control flag
        uint64_t drop_ts = 0;         // kernel drop timestamp (race)
        mutex mtx;                    // lock for drop_ts
        condition_variable cv;        // for reading drop_ts
        unique_lock<mutex> lck(mtx);
        unique_ptr<thread> dm_thread;

        auto dropmon_func = [&]() {
            while (!stop_dm) {
                auto ts = dm.get_drop_ts();

                if (ts) {
                    unique_lock<mutex> lck(mtx);
                    drop_ts = ts;
                    cv.notify_all();
                }
            }
        };

        auto start_dm_thread = [&]() {
            REQUIRE(!dm_thread);
            dm_thread = make_unique<thread>(dropmon_func);
        };

        auto stop_dm_thread = [&]() {
            if (dm_thread) {
                stop_dm = true;
                pthread_kill(dm_thread->native_handle(), SIGUSR1);
                if (dm_thread->joinable()) {
                    dm_thread->join();
                }
                dm_thread.reset();
                stop_dm = false;
            }
        };

        // Ping request packet from node1 to node2
        Packet pkt(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                   PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        REQUIRE_NOTHROW(start_dm_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        size_t nwrite;
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        lck.unlock();
        REQUIRE_NOTHROW(stop_dm_thread());
        lck.lock();
        REQUIRE_NOTHROW(dm.stop_listening());

        // Ping request packet from node2 to node1
        pkt = Packet(eth1, "192.168.2.2", "192.168.1.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        REQUIRE_NOTHROW(start_dm_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        lck.unlock();
        REQUIRE_NOTHROW(stop_dm_thread());
        lck.lock();
        REQUIRE_NOTHROW(dm.stop_listening());

        // Ping request packet from node1 to fw
        pkt = Packet(eth0, "192.168.1.2", "192.168.1.1", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Start the dropmon listener thread
        REQUIRE_NOTHROW(dm.start_listening_for(pkt));
        REQUIRE_NOTHROW(start_dm_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop and join the dropmon listener thread
        lck.unlock();
        REQUIRE_NOTHROW(stop_dm_thread());
        lck.lock();
        REQUIRE_NOTHROW(dm.stop_listening());

        // Stop the kernel drop_monitor
        REQUIRE_NOTHROW(dm.stop());
        // Reset and disable the dropmon module
        REQUIRE_NOTHROW(dm.teardown());
        // Reset docker driver object
        REQUIRE_NOTHROW(docker.teardown());

        // Reset signal handler
        sigaction(SIGUSR1, oldaction, nullptr);
    }
}
