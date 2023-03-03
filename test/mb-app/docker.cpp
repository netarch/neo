#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

// a hack so we can modify private fields
#define private public
#include "mb-app/docker-util.hpp"
#include "mb-app/docker.hpp"

TEST_CASE("docker") {
    std::cout << "Creating Docker object" << std::endl;
    Docker docker = Docker();
    docker.container_name = "test";
    docker.image = "hashicorp/http-echo";
    docker.cmd = {R"(-text="hello world")"};

    SECTION("Start Container") {
        std::cout << "Initialize" << std::endl;
        CHECK_NOTHROW(docker.init());

        std::cout << "The pid is " + std::to_string(docker.pid) << std::endl;
        CHECK_FALSE(docker.pid == 0);
    }

    SECTION("Check Exists and Running") {
        auto response =
            DockerUtil::inspect_container_running(docker.container_name);

        std::cout << "running: " << response.first << std::endl;
        std::cout << "exists: " << response.second << std::endl;

        CHECK(response.first);
        CHECK(response.second);
    }

    SECTION("Soft Reset") {
        std::cout << "Reset Docker" << std::endl;
        CHECK_NOTHROW(docker.reset());
    }

    SECTION("Hard Reset") {
        std::cout << "re-init Docker" << std::endl;
        CHECK_NOTHROW(docker.init());

        std::cout << "The pid is " + std::to_string(docker.pid) << std::endl;
        CHECK_FALSE(docker.pid == 0);
    }

    SECTION("Check Exists and Running after hard reset") {
        auto response =
            DockerUtil::inspect_container_running(docker.container_name);

        std::cout << "running: " << response.first << std::endl;
        std::cout << "exists: " << response.second << std::endl;

        CHECK(response.first);
        CHECK(response.second);
    }

    SECTION("Destroy") {
        std::cout << "destroy container" << std::endl;

        auto response = DockerUtil::remove_container(docker.container_name);

        CHECK(response["success"].GetBool());
    }
}
