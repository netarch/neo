#include <memory>
#include <signal.h>
#include <string>

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
    Middlebox *mb = static_cast<Middlebox *>(network.nodes().at("fw"));
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
        REQUIRE_NOTHROW(emu1.init(mb, false));
        CHECK(emu1.mb() == mb);
        CHECK(emu1.node_pkt_hist() == nullptr);
        CHECK(emu2.node_pkt_hist() == nullptr);
    }

    SECTION("Re-initialization") {
        Emulation emu;
        REQUIRE_NOTHROW(emu.init(mb, false));
        REQUIRE_NOTHROW(emu.init(mb, false));
        REQUIRE_NOTHROW(emu.init(mb, false));
    }

    SECTION("Send packets") {
        Emulation emu;
        REQUIRE_NOTHROW(emu.init(mb, false));
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

    SECTION("Rewind") {
        /**
         * nph0 (null) ---- nph1 ---- nph2 ---- nph3
         *    \
         *     + ---- nph4 ---- nph5
         */

        Emulation emu;
        REQUIRE_NOTHROW(emu.init(mb, false));
        unique_ptr<NodePacketHistory> nph0(nullptr);

        // Send a ping req packet from node1 to node2
        auto ping12 = make_unique<Packet>(eth0, "192.168.1.2", "192.168.2.2", 0,
                                          0, 0, 0, PS_ICMP_ECHO_REQ);
        auto nph1 = make_unique<NodePacketHistory>(ping12.get(), nph0.get());
        REQUIRE_NOTHROW(emu.send_pkt(*ping12));
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph1.get()));

        // Send a TCP SYN packet from node1 to node2
        auto syn12 = make_unique<Packet>(eth0, "192.168.1.2", "192.168.2.2",
                                         DYNAMIC_PORT, 80, 0, 0, PS_TCP_INIT_1);
        auto nph2 = make_unique<NodePacketHistory>(syn12.get(), nph1.get());
        REQUIRE_NOTHROW(emu.send_pkt(*syn12));
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph2.get()));

        // Send a ping req packet from node1 to fw
        auto pingfw = make_unique<Packet>(eth0, "192.168.1.2", "192.168.1.1", 0,
                                          0, 0, 0, PS_ICMP_ECHO_REQ);
        auto nph3 = make_unique<NodePacketHistory>(pingfw.get(), nph2.get());
        REQUIRE_NOTHROW(emu.send_pkt(*pingfw));
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph3.get()));

        // Rewind to nph0
        CHECK(emu.rewind(nph0.get()) == 0); // Reset but no injections
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph0.get()));
        CHECK(emu.rewind(nph0.get()) == -1); // No resets at all
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph0.get()));

        // Send a TCP SYN packet from node1 to node2
        auto nph4 = make_unique<NodePacketHistory>(syn12.get(), nph0.get());
        REQUIRE_NOTHROW(emu.send_pkt(*syn12));
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph4.get()));

        // Send a ping req packet from node1 to node2
        auto nph5 = make_unique<NodePacketHistory>(ping12.get(), nph4.get());
        REQUIRE_NOTHROW(emu.send_pkt(*ping12));
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph5.get()));

        // Test a bunch of rewinds
        CHECK(emu.rewind(nph4.get()) == 1); // Rewind to nph4
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph4.get()));
        CHECK(emu.rewind(nph5.get()) == 1); // Rewind to nph5
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph5.get()));
        CHECK(emu.rewind(nph3.get()) == 3); // Rewind to nph3
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph3.get()));
        CHECK(emu.rewind(nph3.get()) == -1); // Rewind to nph3
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph3.get()));
        CHECK(emu.rewind(nph2.get()) == 2); // Rewind to nph2
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph2.get()));
        CHECK(emu.rewind(nph1.get()) == 1); // Rewind to nph1
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph1.get()));
        CHECK(emu.rewind(nph3.get()) == 2); // Rewind to nph3
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph3.get()));
        CHECK(emu.rewind(nph5.get()) == 2); // Rewind to nph5
        REQUIRE_NOTHROW(emu.node_pkt_hist(nph5.get()));
    }

    // Reset signal handler
    sigaction(SIGUSR1, oldaction, nullptr);
}
