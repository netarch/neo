#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <iostream>

// a hack so we can modify private fields
#define private public
#include "mb-app/docker.hpp"

TEST_CASE("docker") {

    SECTION("test") {
        INFO("start testing");
        std::cout << "start testing" << std::endl;
        std::cerr << "start testing" << std::endl;
        CHECK(true);
    }

    SECTION("Start Container") {
        INFO("Creating Docker object");
        Docker docker = Docker();

        INFO("Setting fields");
        docker.container_name = "test";
        docker.image = "hashicorp/http-echo";
        docker.cmd = {R"(-text="hello world")"};

        INFO("Initialize");
        CHECK_NOTHROW(docker.init());

        INFO("The pid is" + std::to_string(docker.pid));
        CHECK_FALSE(docker.pid == 0);
    }
}
