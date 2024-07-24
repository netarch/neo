#include <chrono>
#include <list>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"
#include "protocols.hpp"

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
    REQUIRE_NOTHROW(node = static_cast<DockerNode *>(network.nodes().at("fw")));
    REQUIRE(node);

    chrono::seconds timeout(1);
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

    SECTION("Get netns inode number") {
        REQUIRE_NOTHROW(docker.init());
        CHECK(docker.netns_ino() > 0);
        REQUIRE_NOTHROW(docker.teardown());
        CHECK_THROWS_WITH(docker.netns_ino(), "Container isn't running");
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
        REQUIRE_NOTHROW(docker.pause());

        atomic<bool> stop_recv = false;
        list<Packet> recv_pkts;
        size_t num_pkts;
        mutex mtx;             // lock for recv_pkts
        condition_variable cv; // for reading recv_pkts
        unique_lock<mutex> lck(mtx);
        Interface *eth0 = nullptr;
        Interface *eth1 = nullptr;
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
        Packet compare_pkt;

        // Send the ping packet
        REQUIRE_NOTHROW(docker.unpause());
        size_t nwrite = docker.inject_packet(pkt);
        CHECK(nwrite == 42);

        // Receive packets
        do {
            num_pkts = recv_pkts.size();
            cv.wait_for(lck, timeout);
        } while (recv_pkts.size() > num_pkts);
        REQUIRE_NOTHROW(docker.pause());

        // Process the received packets
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = pkt);
        REQUIRE_NOTHROW(compare_pkt.set_intf(eth1));
        CHECK(recv_pkts.front() == compare_pkt);
        recv_pkts.clear();

        // Send a TCP SYN packet
        REQUIRE_NOTHROW(docker.unpause());
        pkt.set_src_port(DYNAMIC_PORT);
        pkt.set_dst_port(80);
        pkt.set_proto_state(PS_TCP_INIT_1);
        nwrite = docker.inject_packet(pkt);
        CHECK(nwrite == 54);

        // Receive packets
        do {
            num_pkts = recv_pkts.size();
            cv.wait_for(lck, timeout);
        } while (recv_pkts.size() > num_pkts);
        REQUIRE_NOTHROW(docker.pause());

        // Process the received packets
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = pkt);
        REQUIRE_NOTHROW(compare_pkt.set_intf(eth1));
        REQUIRE_NOTHROW(
            compare_pkt.set_proto_state(recv_pkts.front().get_proto_state()));
        CHECK(recv_pkts.front() == compare_pkt);
        recv_pkts.clear();

        // Send a ping packet to fw
        REQUIRE_NOTHROW(docker.unpause());
        pkt.set_dst_ip("192.168.1.1");
        pkt.set_src_port(0);
        pkt.set_dst_port(0);
        pkt.set_proto_state(PS_ICMP_ECHO_REQ);
        nwrite = docker.inject_packet(pkt);
        CHECK(nwrite == 42);

        // Receive packets
        do {
            num_pkts = recv_pkts.size();
            cv.wait_for(lck, timeout);
        } while (recv_pkts.size() > num_pkts);
        REQUIRE_NOTHROW(docker.pause());

        // Process the received packets
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = pkt);
        REQUIRE_NOTHROW(compare_pkt.set_src_ip(pkt.get_dst_ip()));
        REQUIRE_NOTHROW(compare_pkt.set_dst_ip(pkt.get_src_ip()));
        REQUIRE_NOTHROW(compare_pkt.set_proto_state(PS_ICMP_ECHO_REP));
        CHECK(recv_pkts.front() == compare_pkt);
        recv_pkts.clear();

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
