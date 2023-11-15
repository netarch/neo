#include <format>
#include <string>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "configparser.hpp"
#include "dockerapi.hpp"
#include "dockernode.hpp"
#include "network.hpp"
#include "plankton.hpp"

using namespace std;
using namespace rapidjson;
using Catch::Matchers::ContainsSubstring;

extern string test_data_dir;

TEST_CASE("dockerapi") {
    auto &plankton = Plankton::get();
    plankton.reset();
    const string inputfn = test_data_dir + "/docker.toml";
    REQUIRE_NOTHROW(ConfigParser().parse(inputfn, plankton));
    const auto &network = plankton.network();
    DockerNode *node;
    REQUIRE_NOTHROW(node = static_cast<DockerNode *>(network.nodes().at("fw")));
    REQUIRE(node);

    const string &cntr_name = std::format("{}.{}", getpid(), node->get_name());
    DockerAPI dapi;

    SECTION("Container API") {
        REQUIRE_NOTHROW(dapi.pull(node->image()));
        REQUIRE_NOTHROW(dapi.run(cntr_name, *node));
        REQUIRE(dapi.stop_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.start_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.restart_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.pause_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.unpause_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.kill_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.remove_cntr(cntr_name)["success"].GetBool());
    }

    SECTION("Image API") {
        REQUIRE(dapi.create_img(node->image())["success"].GetBool());
        REQUIRE(dapi.inspect_img(node->image())["success"].GetBool());
        REQUIRE(dapi.rm_img(node->image())["success"].GetBool());
    }

    SECTION("Exec API") {
        REQUIRE_NOTHROW(dapi.pull(node->image()));
        REQUIRE_NOTHROW(dapi.run(cntr_name, *node));
        REQUIRE(dapi.stop_cntr(cntr_name)["success"].GetBool());
        CHECK_THROWS_WITH(
            dapi.exec(cntr_name, node->env_vars(), {"/bin/sh"}, "/"),
            ContainsSubstring("is not running"));
        REQUIRE(dapi.start_cntr(cntr_name)["success"].GetBool());
        CHECK_NOTHROW(dapi.exec(cntr_name, node->env_vars(), {"/bin/ls"}, "/"));
        REQUIRE(dapi.kill_cntr(cntr_name)["success"].GetBool());
        REQUIRE(dapi.remove_cntr(cntr_name)["success"].GetBool());
    }

    SECTION("System API") {
        REQUIRE(dapi.info()["success"].GetBool());
    }
}
