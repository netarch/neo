#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

class DockerNode;

/**
 * Helper class for global RAII for curl
 */
class DockerAPI_GLOBAL {
public:
    DockerAPI_GLOBAL();
    ~DockerAPI_GLOBAL();
};

/**
 * Synchronous API for interacting with docker daemons
 * https://docs.docker.com/engine/api/latest/
 */
class DockerAPI {
private:
    // Path to docker socket. `docker` service must be up and the running user
    // must be root or in docker group
    std::string _socket_path;

    // Specifies docker API version.
    // https://docs.docker.com/engine/api/v1.42/#section/Versioning
    static const std::string _api_version;
    std::string _uri_prefix;

    CURL *_curl;
    struct curl_slist *_header;

    static const rapidjson::Document _empty_json;
    static const DockerAPI_GLOBAL _global_raii;

    enum class method {
        GET,
        POST,
        DELETE,
        PUT
    };

    static inline const std::string method_to_str(method m) {
        static const std::unordered_map<method, std::string> methods = {
            {   method::GET,    "GET"},
            {  method::POST,   "POST"},
            {method::DELETE, "DELETE"},
            {   method::PUT,    "PUT"},
        };

        return methods.at(m);
    };

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
     * @param request_body the request body (only effective if method=POST)
     * @return a json Document of the response.
     */
    rapidjson::Document
    send_curl_request(method method,
                      const std::string &path,
                      const rapidjson::Document &request_body = _empty_json);

    /**
     * Writes the body content into a string as specified by CURLOPT_WRITEDATA
     * (https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c)
     */
    static size_t
    write_cb(void *contents, size_t size, size_t nmemb, void *userp);

public:
    DockerAPI();
    DockerAPI(const std::string &socket_path);
    ~DockerAPI();

    const decltype(_socket_path) &socket_path() const { return _socket_path; }
    const decltype(_uri_prefix) &uri_prefix() const { return _uri_prefix; }

    static inline std::string json_str(const rapidjson::Document &d) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);
        return std::string(buffer.GetString());
    }

    static inline std::string json_str(const rapidjson::Value &v) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        v.Accept(writer);
        return std::string(buffer.GetString());
    }

    // Container API
    // https://docs.docker.com/engine/api/v1.42/#tag/Container
    rapidjson::Document create_cntr(const std::string &name,
                                    const DockerNode &node);
    rapidjson::Document inspect_cntr(const std::string &name);
    rapidjson::Document start_cntr(const std::string &name);
    rapidjson::Document stop_cntr(const std::string &name);
    rapidjson::Document restart_cntr(const std::string &name);
    rapidjson::Document kill_cntr(const std::string &name);
    rapidjson::Document pause_cntr(const std::string &name);
    rapidjson::Document unpause_cntr(const std::string &name);
    rapidjson::Document remove_cntr(const std::string &name);

    // Image API
    // https://docs.docker.com/engine/api/v1.42/#tag/Image
    rapidjson::Document create_img(const std::string &img_name);
    rapidjson::Document inspect_img(const std::string &img_name);
    rapidjson::Document rm_img(const std::string &img_name);

    // Exec API
    // https://docs.docker.com/engine/api/v1.42/#tag/Exec
    rapidjson::Document
    create_exec(const std::string &cntr_name,
                const std::unordered_map<std::string, std::string> &envs,
                const std::vector<std::string> &cmd,
                const std::string &working_dir);
    rapidjson::Document start_exec(const std::string &exec_id);
    rapidjson::Document inspect_exec(const std::string &exec_id);

    // System API
    // https://docs.docker.com/engine/api/v1.42/#tag/System
    rapidjson::Document info();

    // Helper functions
    rapidjson::Document pull(const std::string &img_name);
    int run(const std::string &cntr_name, const DockerNode &node);
    int get_cntr_pid(const std::string &cntr_name);
    bool is_cntr_running(const std::string &cntr_name);
    std::pair<int, std::string>
    exec(const std::string &cntr_name,
         const std::unordered_map<std::string, std::string> &envs,
         const std::vector<std::string> &cmd,
         const std::string &working_dir);
    bool is_exec_running(const std::string &exec_id);
};
