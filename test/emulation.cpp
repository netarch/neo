#include <signal.h>

#include <catch2/catch_test_macros.hpp>

#include "configparser.hpp"
#include "emulation.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"
#include "protocols.hpp"

using namespace std;

extern string test_data_dir;

TEST_CASE("emulation") {
    auto &plankton = Plankton::get();
    plankton.reset();
    const string inputfn = test_data_dir + "/docker.toml";
    REQUIRE_NOTHROW(ConfigParser().parse(inputfn, plankton));
    const auto &network = plankton.network();
    Middlebox *mb = static_cast<Middlebox *>(network.get_nodes().at("fw"));
    REQUIRE(mb);

    Interface *eth0;
    Interface *eth1;
    REQUIRE_NOTHROW(eth0 = mb->get_intfs().at("eth0"));
    REQUIRE_NOTHROW(eth1 = mb->get_intfs().at("eth1"));
    REQUIRE(eth0 != nullptr);
    REQUIRE(eth1 != nullptr);

    // Register signal handler to nullify SIGUSR1
    struct sigaction action, *oldaction = nullptr;
    action.sa_handler = [](int) {};
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGUSR1);
    action.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGUSR1, &action, oldaction);

    SECTION("Constructor/Destructor") {
        Emulation emu1, emu2;
        REQUIRE_NOTHROW(emu1.init(mb));
        CHECK(emu1.mb() == mb);
        CHECK(emu1.node_pkt_hist() == nullptr);
        CHECK(emu2.node_pkt_hist() == nullptr);
    }

    SECTION("Re-initialization") {
        Emulation emu;
        REQUIRE_NOTHROW(emu.init(mb));
        REQUIRE_NOTHROW(emu.init(mb));
        REQUIRE_NOTHROW(emu.init(mb));
    }

    SECTION("Send packets") {
        Emulation emu;
        REQUIRE_NOTHROW(emu.init(mb));
        Packet send_pkt, compare_pkt;
        list<Packet> recv_pkts;

        // Send a ping req packet from node1 to node2
        send_pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", 0, 0, 0, 0,
                          PS_ICMP_ECHO_REQ);
        recv_pkts = emu.send_pkt(send_pkt);
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = send_pkt);
        REQUIRE_NOTHROW(compare_pkt.set_intf(eth1));
        CHECK(recv_pkts.front() == compare_pkt);

        // Send a TCP SYN packet from node1 to node2
        send_pkt = Packet(eth0, "192.168.1.2", "192.168.2.2", DYNAMIC_PORT, 80,
                          0, 0, PS_TCP_INIT_1);
        recv_pkts = emu.send_pkt(send_pkt);
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = send_pkt);
        REQUIRE_NOTHROW(compare_pkt.set_intf(eth1));
        REQUIRE_NOTHROW(
            compare_pkt.set_proto_state(recv_pkts.front().get_proto_state()));
        CHECK(recv_pkts.front() == compare_pkt);

        // Send a ping req packet from node1 to fw
        send_pkt = Packet(eth0, "192.168.1.2", "192.168.1.1", 0, 0, 0, 0,
                          PS_ICMP_ECHO_REQ);
        recv_pkts = emu.send_pkt(send_pkt);
        REQUIRE(recv_pkts.size() == 1);
        REQUIRE_NOTHROW(compare_pkt = send_pkt);
        REQUIRE_NOTHROW(compare_pkt.set_src_ip(send_pkt.get_dst_ip()));
        REQUIRE_NOTHROW(compare_pkt.set_dst_ip(send_pkt.get_src_ip()));
        REQUIRE_NOTHROW(compare_pkt.set_proto_state(PS_ICMP_ECHO_REP));
        CHECK(recv_pkts.front() == compare_pkt);
    }

    // SECTION("Rewind") {
    //     ;
    // }

    // Reset signal handler
    sigaction(SIGUSR1, oldaction, nullptr);
}
