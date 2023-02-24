#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <iostream>
#include "mb-app/docker-util.hpp"

TEST_CASE("docker-utils") {
    using namespace rapidjson;

    SECTION("System Info") {
        auto response = DockerUtil::system_info();

        // serialize to text
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        response.Accept(writer);

        std::cout << "system info" << std::endl;
        std::cout << buffer.GetString() << std::endl;

        CHECK(response["success"].GetBool());
    }

    SECTION("System Info") {
        auto response = DockerUtil::inspect_container("naughty_lovelace");

        if (!response.HasMember("data") || !response["data"].HasMember("State") || !response["data"]["State"].HasMember("Pid")) {
            // field does not exist, error
            exit(1);
        }
        rapidjson::Value& pid = response["data"]["State"]["Pid"];

        // serialize to text
//        StringBuffer buffer;
//        Writer<StringBuffer> writer(buffer);
//        pid.Accept(writer);

        std::cout << "system info" << std::endl;
        std::cout << pid.GetInt() << std::endl;

        CHECK(response["success"].GetBool());
    }

}
