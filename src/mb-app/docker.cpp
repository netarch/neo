#include "docker.hpp"

#include <curl/curl.h>
#include <iostream>

#include "docker-util.hpp"

Docker::Docker() {
    pid = 0;
    //    curl_global_init(CURL_GLOBAL_ALL);
}

Docker::~Docker() {
    //    curl_global_cleanup();
}

void Docker::init() {
    // start docker container with no network
    Logger::info("docker container initializing");

    // if container exists and active, kill
    // if container exists, delete

    /*
    {
        "Cmd": [
           "-text=hello world"
        ],
        "Image": "hashicorp/http-echo",
        "HostConfig": {
           "NetworkMode": "none"
        }
    }
     */

    rapidjson::Document request_body = {};
    request_body.SetObject();

    rapidjson::Value val;
    val.SetString(image.c_str(), image.length());
    request_body.AddMember("Image", val, request_body.GetAllocator());

    rapidjson::Value HostConfig = {};
    HostConfig.SetObject();
    HostConfig.AddMember("NetworkMode", "none", request_body.GetAllocator());
    request_body.AddMember("HostConfig", HostConfig,
                           request_body.GetAllocator());

    //    std::cout << request_body << std::endl;

    rapidjson::Value array;
    array.SetArray();
    for (const std::string &str : cmd) {
        val.SetString(str.c_str(), str.length());
        array.PushBack(val, request_body.GetAllocator());
    }
    request_body.AddMember("Cmd", array, request_body.GetAllocator());

    DockerUtil::create_container(request_body, container_name);
    DockerUtil::start_container(container_name);

    pid = DockerUtil::inspect_container_pid(container_name);
    Logger::info("docker container has pid " + std::to_string(pid));
}

void Docker::reset() {
    // POST /restart ?
}
