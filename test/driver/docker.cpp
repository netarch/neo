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

    Docker docker(node);

    // SECTION("Start Container") {
    //     // std::cout << "Initialize" << std::endl;
    //     CHECK_NOTHROW(docker.init());

    //     // std::cout << "The pid is " + std::to_string(docker.pid) <<
    //     std::endl; CHECK_FALSE(docker.pid == 0);
    // }

    // SECTION("Check Exists and Running") {
    //     auto response =
    //         DockerUtil::inspect_container_running(docker.container_name);

    //     std::cout << "running: " << response.first << std::endl;
    //     std::cout << "exists: " << response.second << std::endl;

    //     CHECK(response.first);
    //     CHECK(response.second);
    // }

    // SECTION("Soft Reset") {
    //     std::cout << "Reset Docker" << std::endl;
    //     CHECK_NOTHROW(docker.reset());
    // }

    // SECTION("Hard Reset") {
    //     std::cout << "re-init Docker" << std::endl;
    //     CHECK_NOTHROW(docker.init());

    //     std::cout << "The pid is " + std::to_string(docker.pid) << std::endl;
    //     CHECK_FALSE(docker.pid == 0);
    // }

    // SECTION("Check Exists and Running after hard reset") {
    //     auto response =
    //         DockerUtil::inspect_container_running(docker.container_name);

    //     std::cout << "running: " << response.first << std::endl;
    //     std::cout << "exists: " << response.second << std::endl;

    //     CHECK(response.first);
    //     CHECK(response.second);
    // }

    // SECTION("Destroy") {
    //     std::cout << "destroy container" << std::endl;

    //     auto response = DockerUtil::remove_container(docker.container_name);

    //     CHECK(response["success"].GetBool());
    // }
}
