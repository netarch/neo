#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "droptrace.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"
#include "protocols.hpp"

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
        REQUIRE_NOTHROW(dt.start_listening_for(Packet(), nullptr));
        CHECK(dt.get_drop_ts() == 0);
        REQUIRE_NOTHROW(dt.stop_listening());
        REQUIRE_NOTHROW(dt.teardown());
    }

    SECTION("Exceptions") {
        // Ping request packet from node1 to node2
        Packet pkt(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                   PS_ICMP_ECHO_REQ);

        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.start());
        CHECK_THROWS_WITH(dt.start(), "Another BPF progam is already loaded");
        CHECK_THROWS_WITH(dt.start_listening_for(Packet(), nullptr),
                          "Empty target packet");
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, nullptr));
        CHECK_THROWS_WITH(dt.start_listening_for(pkt, nullptr),
                          "Ring buffer already opened");
        REQUIRE_NOTHROW(dt.stop_listening());
        REQUIRE_NOTHROW(dt.stop());
        REQUIRE_NOTHROW(dt.teardown());
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
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
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

        // Ping request packet from node2 to node1
        pkt = Packet(eth1, "192.168.2.2", "192.168.1.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
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

        // Ping request packet from node1 to fw
        pkt = Packet(eth0, "192.168.1.2", "192.168.1.1", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
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

        // TCP SYN packet from node1 to node2
        pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", DYNAMIC_PORT, 80, 0, 0,
                     PS_TCP_INIT_1);
        // Attach the BPF program
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 54);
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
        REQUIRE_NOTHROW(dt.init());
        REQUIRE_NOTHROW(dt.start());

        atomic<bool> stop_dt = false; // loop control flag
        uint64_t drop_ts = 0;         // kernel drop timestamp (race)
        mutex mtx;                    // lock for drop_ts
        condition_variable cv;        // for reading drop_ts
        unique_lock<mutex> lck(mtx);
        unique_ptr<thread> dt_thread;

        auto droptrace_func = [&]() {
            while (!stop_dt) {
                auto ts = dt.get_drop_ts();

                if (ts) {
                    unique_lock<mutex> lck(mtx);
                    drop_ts = ts;
                    cv.notify_all();
                }
            }
        };

        auto start_dt_thread = [&]() {
            REQUIRE(!dt_thread);
            dt_thread = make_unique<thread>(droptrace_func);
        };

        auto stop_dt_thread = [&]() {
            if (dt_thread) {
                stop_dt = true;
                dt.unblock(*dt_thread);
                if (dt_thread->joinable()) {
                    dt_thread->join();
                }
                dt_thread.reset();
                stop_dt = false;
            }
        };

        Packet pkt;
        size_t nwrite;

        // Ping request packet from node1 to node2
        pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program and start the droptrace thread
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
        REQUIRE_NOTHROW(start_dt_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop/join the droptrace thread and detach the BPF program
        lck.unlock();
        REQUIRE_NOTHROW(stop_dt_thread());
        lck.lock();
        REQUIRE_NOTHROW(dt.stop_listening());

        // Ping request packet from node2 to node1
        pkt = Packet(eth1, "192.168.2.2", "192.168.1.2", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program and start the droptrace thread
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
        REQUIRE_NOTHROW(start_dt_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop/join the droptrace thread and detach the BPF program
        lck.unlock();
        REQUIRE_NOTHROW(stop_dt_thread());
        lck.lock();
        REQUIRE_NOTHROW(dt.stop_listening());

        // Ping request packet from node1 to fw
        pkt = Packet(eth0, "192.168.1.2", "192.168.1.1", 0, 0, 0, 0,
                     PS_ICMP_ECHO_REQ);
        // Attach the BPF program and start the droptrace thread
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
        REQUIRE_NOTHROW(start_dt_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 42);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop/join the droptrace thread and detach the BPF program
        lck.unlock();
        REQUIRE_NOTHROW(stop_dt_thread());
        lck.lock();
        REQUIRE_NOTHROW(dt.stop_listening());

        // TCP SYN packet from node1 to node2
        pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", DYNAMIC_PORT, 80, 0, 0,
                     PS_TCP_INIT_1);
        // Attach the BPF program and start the droptrace thread
        REQUIRE_NOTHROW(dt.start_listening_for(pkt, &docker));
        REQUIRE_NOTHROW(start_dt_thread());
        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        REQUIRE_NOTHROW(nwrite = docker.inject_packet(pkt));
        CHECK(nwrite == 54);
        // Get the kernel drop timestamp (blocking)
        drop_ts = 0;
        cv.wait_for(lck, timeout);
        CHECK(drop_ts > 0);
        REQUIRE_NOTHROW(docker.pause());
        // Stop/join the droptrace thread and detach the BPF program
        lck.unlock();
        REQUIRE_NOTHROW(stop_dt_thread());
        lck.lock();
        REQUIRE_NOTHROW(dt.stop_listening());

        REQUIRE_NOTHROW(dt.stop()); // Remove the BPF program
        REQUIRE_NOTHROW(dt.teardown());
        REQUIRE_NOTHROW(docker.teardown());

        // Reset signal handler
        sigaction(SIGUSR1, oldaction, nullptr);
    }
}
