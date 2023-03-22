#include <catch2/catch_test_macros.hpp>

#include "configparser.hpp"
#include "dockernode.hpp"
#include "driver/docker.hpp"
#include "network.hpp"
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

    Docker docker(node, false);

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
        // Check if the inodes of /proc/<docker.pid()>/ns/{mnt,net} are the same
        // as /proc/self/ns/{mnt,net} after docker.enterns()
        // man 2 stat
        // docker.enterns();
        // cerr << docker.pid() << endl;
        // system("/bin/sh");
        // docker.leavens();
        REQUIRE_NOTHROW(docker.teardown());
    }
}
