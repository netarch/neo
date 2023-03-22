#include <chrono>
#include <list>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"

using namespace std;
using namespace rapidjson;

extern string test_data_dir;

TEST_CASE("docker") {
    auto &plankton = Plankton::get();
    plankton.reset();
    const string inputfn = test_data_dir + "/docker.toml";
    REQUIRE_NOTHROW(ConfigParser().parse(inputfn, plankton));
    const auto &network = plankton.network();
    DockerNode *node;
    REQUIRE_NOTHROW(
        node = static_cast<DockerNode *>(network.get_nodes().at("fw")));
    REQUIRE(node);

    Docker docker(node, /* log_pkts */ false);

    SECTION("Start and terminate container") {
        REQUIRE_NOTHROW(docker.init());
        CHECK(docker.pid() > 0);
        REQUIRE_NOTHROW(docker.teardown());
    }

    SECTION("Hard reset") {
        REQUIRE_NOTHROW(docker.init());
        CHECK(docker.pid() > 0);
        REQUIRE_NOTHROW(docker.init());
        CHECK(docker.pid() > 0);
        REQUIRE_NOTHROW(docker.teardown());
    }

    SECTION("Soft reset") {
        REQUIRE_NOTHROW(docker.init());
        REQUIRE(docker.pid() > 0);
        REQUIRE_NOTHROW(docker.reset());
        REQUIRE_NOTHROW(docker.enterns(/* mnt */ true));
        REQUIRE_NOTHROW(docker.leavens(/* mnt */ true));
        REQUIRE_NOTHROW(docker.teardown());
    }

    SECTION("Send and read packets") {
        // Register signal handler to nullify SIGUSR1
        struct sigaction action, *oldaction = nullptr;
        action.sa_handler = [](int) {};
        sigemptyset(&action.sa_mask);
        sigaddset(&action.sa_mask, SIGUSR1);
        action.sa_flags = SA_NOCLDSTOP;
        sigaction(SIGUSR1, &action, oldaction);

        REQUIRE_NOTHROW(docker.init());

        atomic<bool> stop_recv = false;
        list<Packet> recv_pkts;
        size_t num_pkts;
        mutex mtx;             // lock for recv_pkts
        condition_variable cv; // for reading recv_pkts
        unique_lock<mutex> lck(mtx);
        Interface *eth0;
        Interface *eth1;
        REQUIRE_NOTHROW(eth0 = node->get_intfs().at("eth0"));
        REQUIRE_NOTHROW(eth1 = node->get_intfs().at("eth1"));

        // Set up the recv thread
        thread recv_thread([&]() {
            while (!stop_recv) {
                auto pkts = docker.read_packets();

                if (!pkts.empty()) {
                    unique_lock<mutex> lck(mtx);
                    recv_pkts.splice(recv_pkts.end(), pkts);
                    cv.notify_all();
                }
            }
        });

        // Ping request packet from node1 to node2
        Packet pkt(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                   PS_ICMP_ECHO_REQ);

        // Send the packet
        size_t nwrite = docker.inject_packet(pkt);
        CHECK(nwrite == 42);

        // Receive packets
        do {
            num_pkts = recv_pkts.size();
            cv.wait_for(lck, chrono::microseconds(5000));
        } while (recv_pkts.size() > num_pkts);

        // Process the received packets
        REQUIRE(recv_pkts.size() == 1);
        CHECK(recv_pkts.front().get_intf() == eth1);
        REQUIRE_NOTHROW(recv_pkts.front().set_intf(eth0));
        CHECK(recv_pkts.front() == pkt);

        // Stop the recv thread
        stop_recv = true;
        lck.unlock();
        pthread_kill(recv_thread.native_handle(), SIGUSR1);
        if (recv_thread.joinable()) {
            recv_thread.join();
        }
        lck.lock();

        // Reset signal handler
        sigaction(SIGUSR1, oldaction, nullptr);
    }
}
