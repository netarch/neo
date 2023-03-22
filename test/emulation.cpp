#include <signal.h>

#include <catch2/catch_test_macros.hpp>

#include "configparser.hpp"
#include "emulation.hpp"
#include "middlebox.hpp"
#include "network.hpp"
#include "packet.hpp"
#include "plankton.hpp"

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

    // SECTION("Send packets") {
    //     // inject packets
    //     Packet packet(mb_eth0, "192.168.1.2", "192.168.2.2", 49152, 80, 0, 0,
    //                 PS_TCP_INIT_1);
    //     assert(emulation._dropmon == false);
    //     emulation._dropmon = true; // temporarily disable timeout
    //     for (int i = 0; i < 10; ++i) {
    //         emulation.send_pkt(packet);
    //         packet.set_src_port(packet.get_src_port() + 1);
    //     }
    //     emulation._dropmon = false;

    //     // calculate latency average and mean deviation
    //     const vector<pair<uint64_t, uint64_t>> &pkt_latencies =
    //         Stats::get_pkt_latencies();
    //     long long avg = 0;
    //     for (const auto &lat : pkt_latencies) {
    //         avg += lat.second / 1000 + 1;
    //     }
    //     avg /= pkt_latencies.size();
    //     Config::latency_avg = chrono::microseconds(avg);
    //     long long mdev = 0;
    //     for (const auto &lat : pkt_latencies) {
    //         mdev += abs((long long)(lat.second / 1000 + 1) - avg);
    //     }
    //     mdev /= pkt_latencies.size();
    //     Config::latency_mdev = chrono::microseconds(mdev);
    // }

    // SECTION("Rewind") {
    //     ;
    // }

    // Reset signal handler
    sigaction(SIGUSR1, oldaction, nullptr);
}
