#pragma once

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

class DockerUtil {
public:
    static rapidjson::Document system_info();

    [[maybe_unused]] static rapidjson::Document
    create_macvlan_network(const std::string &name,
                           const std::string &subnet,
                           const std::string &gateway,
                           const std::string &parent);

    static rapidjson::Document
    create_container(rapidjson::Document &request_body,
                     const std::string &name = "");

    static rapidjson::Document
    start_container(const std::string &container_name);

    static rapidjson::Document
    stop_container(const std::string &container_name);

    static rapidjson::Document
    restart_container(const std::string &container_name);

    static rapidjson::Document
    kill_container(const std::string &container_name);

    static rapidjson::Document
    remove_container(const std::string &container_name);

    static rapidjson::Document inspect_container(const std::string &name);

    static int inspect_container_pid(const std::string &name);

    // returns <running, exists>
    static std::pair<bool, bool>
    inspect_container_running(const std::string &name);

private:
    typedef enum {
        GET,
        POST,
        DELETE,
        PUT
    } Method;

    static inline const std::string method2str(Method m) {
        std::string methods[]{
            "GET",
            "POST",
            "DELETE",
            "PUT",
        };

        return methods[m];
    };

    static const rapidjson::Document empty_json;

    // path to docker socket. `docker` service must be up and the running user
    // must be root or in docker group
    static constexpr std::string_view uri_path = "/var/run/docker.sock";

    // specifies docker version. Assumes using unix socket. the "docker" will
    // become HTTP Host header, only the path matters
    static constexpr std::string_view uri_prefix = "http://docker/v1.41";

    /**
     * Sends request to the Docker API Engine. Then parses the output as JSON.
     *
     * Note that query parameter should be included within path.
     * While request body (when the HTTP method is POST) are handled separately
     * with the request_body parameter.
     *
     * Always return a JSON document of the following format
     * {
     *      success: [bool],
     *      code: [number],  // the HTTP response code
     *      data: [object],  // the Docker API response
     * }
     *
     * @param method HTTP method
     * @param path path of API endpoint
     * @param success_code success code (may not be 200!)
     * @param request_body the request body (only effective if method=POST)
     * @return a json Document of the response.
     */
    static rapidjson::Document
    executeCurlRequest(Method method,
                       const std::string &path,
                       unsigned success_code = 200,
                       const rapidjson::Document &request_body = empty_json);

    /**
     * From
     * https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
     *
     * Writes the body content into a string as specified by CURLOPT_WRITEDATA
     */
    static size_t
    WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }
};
