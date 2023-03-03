//
// Created by yechs on 1/17/23.
//

#include "docker-util.hpp"

#include <iostream>

#include <curl/curl.h>

const rapidjson::Document DockerUtil::empty_json = {};

rapidjson::Document DockerUtil::system_info() {
    std::string path = "/info";
    return executeCurlRequest(GET, path, 200, empty_json);
}

// get the PID of a container
// curl --unix-socket /var/run/docker.sock
// http://localhost/containers/mb-app-1/json
rapidjson::Document DockerUtil::inspect_container(const std::string &name) {
    std::string path = "/containers/" + name + "/json";
    return executeCurlRequest(GET, path, 200, empty_json);
}

int DockerUtil::inspect_container_pid(const std::string &name) {
    auto response = inspect_container(name);

    if (!response["success"].GetBool()) {
        // error
        return -1;
    }

    if (!response.HasMember("data") || !response["data"].HasMember("State") ||
        !response["data"]["State"].HasMember("Pid")) {
        // field does not exist, error
        return -2;
    }

    rapidjson::Value &pid = response["data"]["State"]["Pid"];

    if (!pid.IsInt()) {
        return -3;
    }

    return pid.GetInt();
}

// returns <running, exists>
std::pair<bool, bool>
DockerUtil::inspect_container_running(const std::string &name) {
    auto response = inspect_container(name);

    if (!response["success"].GetBool()) {
        // error (probably 404 container does not exist)
        return {false, false};
    }

    if (!response.HasMember("data") || !response["data"].HasMember("State") ||
        !response["data"]["State"].HasMember("Running")) {
        // field does not exist, error
        return {false, false};
    }

    rapidjson::Value &running = response["data"]["State"]["Running"];

    if (!running.IsBool()) {
        return {false, false};
    }

    return {running.GetBool(), true};
}

// docker network create -d macvlan --attachable --subnet=192.168.1.1/25
// --gateway=192.168.1.2 -o parent=watertap0 dockervlan_3
[[maybe_unused]] rapidjson::Document
DockerUtil::create_macvlan_network(const std::string &name,
                                   const std::string &subnet,
                                   const std::string &gateway,
                                   const std::string &parent) {
    std::string path = "/networks/create";

    /*
    "Scope": "local",
    "Driver": "macvlan",
    "EnableIPv6": false,
    "IPAM": {
      "Driver": "default",
      "Options": {},
      "Config": [
        {
          "Subnet": "192.168.1.1/25",
          "Gateway": "192.168.1.2"
        }
      ]
    },
    "Internal": false,
    "Attachable": true,
    "Ingress": false,
    "Options": {
      "parent": "watertap0"
    },
     */

    rapidjson::Document request_body = {};
    request_body.SetObject();
    rapidjson::Value val;
    val.SetString(name.c_str(), name.length());
    request_body.AddMember("Name", val, request_body.GetAllocator());
    request_body.AddMember("Driver", "macvlan", request_body.GetAllocator());
    request_body.AddMember("Attachable", true, request_body.GetAllocator());

    rapidjson::Value IPAM = {};
    IPAM.SetObject();
    IPAM.AddMember("Driver", "default", request_body.GetAllocator());

    rapidjson::Value IPAMConfig = {};
    IPAMConfig.SetObject();
    val.SetString(subnet.c_str(), subnet.length());
    IPAMConfig.AddMember("Subnet", val, request_body.GetAllocator());
    val.SetString(gateway.c_str(), gateway.length());
    IPAMConfig.AddMember("Gateway", val, request_body.GetAllocator());

    rapidjson::Value array;
    array.SetArray();
    array.PushBack(IPAMConfig, request_body.GetAllocator());
    IPAM.AddMember("Config", array, request_body.GetAllocator());

    request_body.AddMember("IPAM", IPAM, request_body.GetAllocator());

    rapidjson::Value options = {};
    options.SetObject();
    val.SetString(parent.c_str(), parent.length());
    options.AddMember("parent", val, request_body.GetAllocator());
    request_body.AddMember("Options", val, request_body.GetAllocator());

    return executeCurlRequest(POST, path, 201, request_body);
}

rapidjson::Document
DockerUtil::create_container(rapidjson::Document &request_body,
                             const std::string &name) {
    std::string path = "/containers/create";
    path += not name.empty() ? "?name=" + name : "";
    return executeCurlRequest(POST, path, 201, request_body);
}

rapidjson::Document
DockerUtil::start_container(const std::string &container_name) {
    std::string path = "/containers/" + container_name + "/start";
    return executeCurlRequest(POST, path, 204);
}
rapidjson::Document
DockerUtil::stop_container(const std::string &container_name) {
    std::string path = "/containers/" + container_name + "/stop";
    return executeCurlRequest(POST, path, 204);
}

rapidjson::Document
DockerUtil::restart_container(const std::string &container_name) {
    std::string path = "/containers/" + container_name + "/restart";
    return executeCurlRequest(POST, path, 204);
}

rapidjson::Document
DockerUtil::kill_container(const std::string &container_name) {
    std::string path = "/containers/" + container_name + "/kill";
    return executeCurlRequest(POST, path, 204);
}

rapidjson::Document
DockerUtil::remove_container(const std::string &container_name) {
    std::string path = "/containers/" + container_name + "?force=true";
    return executeCurlRequest(DELETE, path, 204);
}

rapidjson::Document
DockerUtil::executeCurlRequest(Method method,
                               const std::string &path,
                               unsigned int success_code,
                               const rapidjson::Document &request_body) {
    // initialize curl
    // note that `curl_global_init(CURL_GLOBAL_ALL)` should have already been
    // called
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "err: Cannot initialize curl" << std::endl;
        curl_global_cleanup();
        exit(1);
    }

    // curl: set socket path, request path, and method
    std::string host_uri = std::string(uri_prefix);
    std::string method_str = method2str(method);
    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH,
                     std::string(uri_path).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, (host_uri + path).c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method_str.c_str());

    // curl: set header to send (if any) and receive json
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::string paramString;

    // curl: request body
    if (method == POST) {
        // serialize json document to string
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        request_body.Accept(writer);

        paramString = std::string(buffer.GetString());
        const char *paramChar = paramString.c_str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, paramChar);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(paramChar));
    }

    // curl: response body string
    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // curl: execute request and collect response code
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                  << std::endl;
    unsigned res_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

    // curl: cleanup
    // should also call curl_global_cleanup() at end of program
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // handle the responses
    rapidjson::Document response;
    response.SetObject();
    if (res_code == success_code) {
        response.AddMember("success", true, response.GetAllocator());
    } else {
        response.AddMember("success", false, response.GetAllocator());
    }
    response.AddMember("code", res_code, response.GetAllocator());

    rapidjson::Document resp(&response.GetAllocator());
    resp.Parse(readBuffer.c_str());
    response.AddMember("data", resp, response.GetAllocator());

    return response;
}
