#include "dockerapi.hpp"

#include <curl/curl.h>

#include "dockernode.hpp"
#include "logger.hpp"
#include "protocols.hpp"

using namespace std;
using namespace rapidjson;

DockerAPI_GLOBAL::DockerAPI_GLOBAL() {
    curl_global_init(CURL_GLOBAL_NOTHING);
}

DockerAPI_GLOBAL::~DockerAPI_GLOBAL() {
    curl_global_cleanup();
}

const string DockerAPI::_api_version = "v1.42";
const Document DockerAPI::_empty_json = {};
const DockerAPI_GLOBAL DockerAPI::_global_raii;

Document DockerAPI::send_curl_request(method method,
                                      const string &path,
                                      const Document &request_body) {

    // curl: set request URL and method
    string url = this->_uri_prefix + path;
    string method_str = method_to_str(method);
    curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, method_str.c_str());

    // curl: set HTTP header fields (if any) to send and receive json
    assert(_header == nullptr);
    _header = curl_slist_append(_header, "Accept: application/json");
    if (method == method::POST) {
        _header = curl_slist_append(_header, "Content-Type: application/json");
    }
    curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _header);

    // curl: set POST request body
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    request_body.Accept(writer);

    if (method == method::POST) {
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, buffer.GetString());
    } else {
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, nullptr);
        curl_easy_setopt(_curl, CURLOPT_POST, 0);
    }

    // curl: response body string
    string read_buffer;
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, DockerAPI::write_cb);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &read_buffer);

    // curl: send request and collect response code
    if (CURLcode res = curl_easy_perform(_curl); res != CURLE_OK) {
        logger.error("curl_easy_perform(): " + string(curl_easy_strerror(res)));
    }

    unsigned int res_code = 0;
    curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &res_code);

    // curl: free HTTP header list
    curl_slist_free_all(_header);
    _header = nullptr;

    // handle the responses
    Document response;
    response.SetObject();
    response.AddMember("success", (res_code >= 200 && res_code < 300),
                       response.GetAllocator());
    response.AddMember("code", res_code, response.GetAllocator());

    Document resp(&response.GetAllocator());
    resp.Parse(read_buffer.c_str());
    response.AddMember("data", resp, response.GetAllocator());

    return response;
}

size_t
DockerAPI::write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

DockerAPI::DockerAPI() : DockerAPI("/var/run/docker.sock") {}

DockerAPI::DockerAPI(const string &socket_path)
    : _socket_path(socket_path), _curl(nullptr), _header(nullptr) {

    if (this->_socket_path == "/var/run/docker.sock") {
        this->_uri_prefix = "http://docker/" + DockerAPI::_api_version;
    } else {
        this->_uri_prefix = this->_socket_path + "/" + DockerAPI::_api_version;
        logger.error("Remote docker daemon is not supported yet");
    }

    // Initialize curl. `curl_global_init()` should have already been called
    this->_curl = curl_easy_init();

    if (!this->_curl) {
        logger.error("Failed to initialize curl");
    }

    // curl: set socket path
    curl_easy_setopt(_curl, CURLOPT_UNIX_SOCKET_PATH, _socket_path.c_str());
}

DockerAPI::~DockerAPI() {
    // curl: cleanup the current session handle
    // Should also call curl_global_cleanup() at the end of program
    curl_slist_free_all(_header);
    _header = nullptr;
    curl_easy_cleanup(_curl);
    _curl = nullptr;
}

Document DockerAPI::create_cntr(const string &name, const DockerNode &node) {
    /**
     * {
     *  "Hostname": "<node name>",
     *  "AttachStdin": false,
     *  "AttachStdout": false,
     *  "AttachStderr": false,
     *  "ExposedPorts": {
     *      "<port>/<tcp|udp|sctp>": {},
     *  },
     *  "Env": [
     *      "VAR=value",
     *  ]
     *  "Entrypoint": [
     *      "/http-echo",
     *  ],
     *  "Cmd": [
     *      "-text=hello world",
     *  ],
     *  "Image": "hashicorp/http-echo",
     *  "WorkingDir": "<path>",
     *  "HostConfig": {
     *      "Binds": [
     *          "<host src path>:<container dst path>[:(ro|rw)]",
     *      ],
     *      "NetworkMode": "none",
     *      "Privileged": true,
     *      "Sysctls": {
     *          "net.ipv4.ip_forward": "1",
     *      },
     *  },
     * }
     */

    Document body = {};
    body.SetObject();
    auto &allocator = body.GetAllocator();
    body.AddMember("Hostname", Value(name.c_str(), allocator).Move(),
                   allocator);
    body.AddMember("AttachStdin", false, allocator);
    body.AddMember("AttachStdout", false, allocator);
    body.AddMember("AttachStderr", false, allocator);

    Value exposed_ports(kObjectType);
    for (const auto &[protocol, port] : node.ports()) {
        string port_str = to_string(port) + "/" + proto_str_lower(protocol);
        exposed_ports.AddMember(Value(port_str.c_str(), allocator).Move(),
                                Value().Move(), allocator);
    }
    body.AddMember("ExposedPorts", exposed_ports, allocator);

    Value env(kArrayType);
    for (const auto &[key, val] : node.env_vars()) {
        string env_str = key + "=" + val;
        env.PushBack(Value(env_str.c_str(), allocator).Move(), allocator);
    }
    body.AddMember("Env", env, allocator);

    Value entrypoint(kArrayType);
    assert(!node.cmd().empty());
    entrypoint.PushBack(Value(node.cmd().at(0).c_str(), allocator).Move(),
                        allocator);
    body.AddMember("Entrypoint", entrypoint, allocator);

    Value command(kArrayType);
    for (size_t i = 1; i < node.cmd().size(); ++i) {
        command.PushBack(Value(node.cmd().at(i).c_str(), allocator).Move(),
                         allocator);
    }
    body.AddMember("Cmd", command, allocator);
    body.AddMember("Image", Value(node.image().c_str(), allocator).Move(),
                   allocator);
    body.AddMember("WorkingDir",
                   Value(node.working_dir().c_str(), allocator).Move(),
                   allocator);

    Value host_config(kObjectType);
    Value binds(kArrayType);
    for (const auto &mnt : node.mounts()) {
        string bind_str = mnt.host_path + ":" + mnt.mount_path;
        bind_str += mnt.read_only ? ":ro" : ":rw";
        binds.PushBack(Value(bind_str.c_str(), allocator).Move(), allocator);
    }
    host_config.AddMember("Binds", binds, allocator);
    host_config.AddMember("NetworkMode", "none", allocator);
    host_config.AddMember("Privileged", true, allocator);
    Value sysctls(kObjectType);
    for (const auto &[key, value] : node.sysctls()) {
        sysctls.AddMember(Value(key.c_str(), allocator).Move(),
                          Value(value.c_str(), allocator).Move(), allocator);
    }
    host_config.AddMember("Sysctls", sysctls, allocator);
    body.AddMember("HostConfig", host_config, allocator);

    string path = "/containers/create?name=" + name;
    return this->send_curl_request(method::POST, path, body);
}

Document DockerAPI::inspect_cntr(const string &name) {
    string path = "/containers/" + name + "/json";
    return this->send_curl_request(method::GET, path);
}

Document DockerAPI::start_cntr(const string &name) {
    string path = "/containers/" + name + "/start";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::stop_cntr(const string &name) {
    string path = "/containers/" + name + "/stop";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::restart_cntr(const string &name) {
    string path = "/containers/" + name + "/restart";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::kill_cntr(const string &name) {
    string path = "/containers/" + name + "/kill";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::pause_cntr(const string &name) {
    string path = "/containers/" + name + "/pause";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::unpause_cntr(const string &name) {
    string path = "/containers/" + name + "/unpause";
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::remove_cntr(const string &name) {
    string path = "/containers/" + name + "?v=true&force=true";
    return this->send_curl_request(method::DELETE, path);
}

Document DockerAPI::create_img(const string &img_name) {
    string path = "/images/create?fromImage=" + img_name;
    return this->send_curl_request(method::POST, path);
}

Document DockerAPI::inspect_img(const string &img_name) {
    string path = "/images/" + img_name + "/json";
    return this->send_curl_request(method::GET, path);
}

Document DockerAPI::rm_img(const string &img_name) {
    string path = "/images/" + img_name;
    return this->send_curl_request(method::DELETE, path);
}

Document DockerAPI::create_exec(const string &cntr_name,
                                const unordered_map<string, string> &envs,
                                const vector<string> &cmd,
                                const string &working_dir) {
    /**
     * {
     *  "AttachStdin": false,
     *  "AttachStdout": false,
     *  "AttachStderr": false,
     *  "Tty": false,
     *  "Env": [
     *      "FOO=bar",
     *      "BAZ=quux"
     *  ],
     *  "Cmd": [
     *      "date"
     *  ],
     *  "Privileged": true,
     *  "WorkingDir": "<path>",
     * }
     *
     * Response:
     * {
     *  "Id": "<exec ID>"
     * }
     */

    Document body = {};
    body.SetObject();
    auto &allocator = body.GetAllocator();
    body.AddMember("AttachStdin", false, allocator);
    body.AddMember("AttachStdout", false, allocator);
    body.AddMember("AttachStderr", false, allocator);
    body.AddMember("Tty", false, allocator);

    Value env(kArrayType);
    for (const auto &[key, value] : envs) {
        string env_str = key + "=" + value;
        env.PushBack(Value(env_str.c_str(), allocator).Move(), allocator);
    }
    body.AddMember("Env", env, allocator);

    Value command(kArrayType);
    for (const auto &arg : cmd) {
        command.PushBack(Value(arg.c_str(), allocator).Move(), allocator);
    }
    body.AddMember("Cmd", command, allocator);

    body.AddMember("Privileged", true, allocator);
    body.AddMember("WorkingDir", Value(working_dir.c_str(), allocator).Move(),
                   allocator);

    string path = "/containers/" + cntr_name + "/exec";
    return this->send_curl_request(method::POST, path, body);
}

Document DockerAPI::start_exec(const string &exec_id) {
    /**
     * {
     *  "Detach": true,
     *  "Tty": false,
     * }
     */

    Document body = {};
    body.SetObject();
    auto &allocator = body.GetAllocator();
    body.AddMember("Detach", true, allocator);
    body.AddMember("Tty", false, allocator);

    string path = "/exec/" + exec_id + "/start";
    return this->send_curl_request(method::POST, path, body);
}

Document DockerAPI::inspect_exec(const string &exec_id) {
    string path = "/exec/" + exec_id + "/json";
    return this->send_curl_request(method::GET, path);
}

Document DockerAPI::info() {
    string path = "/info";
    return this->send_curl_request(method::GET, path);
}

Document DockerAPI::pull(const string &img_name) {
    auto res = this->inspect_img(img_name);
    if (res["success"].GetBool()) {
        // Image already exists
        return res;
    }

    res = this->create_img(img_name);
    if (!res["success"].GetBool()) {
        logger.error("create_img failed: " + json_str(res));
    }

    res = this->inspect_img(img_name);
    if (!res["success"].GetBool()) {
        logger.error("Failed to pull " + img_name + ": " + json_str(res));
    }

    return res;
}

int DockerAPI::run(const string &cntr_name, const DockerNode &node) {
    auto res = this->create_cntr(cntr_name, node);
    if (!res["success"].GetBool()) {
        logger.error("create_cntr: " + json_str(res));
    }

    res = this->start_cntr(cntr_name);
    if (!res["success"].GetBool()) {
        logger.error("start_cntr: " + json_str(res));
    }

    return this->get_cntr_pid(cntr_name);
}

int DockerAPI::get_cntr_pid(const string &cntr_name) {
    auto res = this->inspect_cntr(cntr_name);
    if (!res["success"].GetBool()) {
        logger.error("inspect_cntr: " + json_str(res));
    }

    // Note that a running container can be paused. The Running and Paused
    // booleans are not mutually exclusive
    if (!res["data"]["State"]["Running"].GetBool()) {
        logger.error("Container isn't running: " + json_str(res));
    }

    if (!res.HasMember("data") || !res["data"].HasMember("State") ||
        !res["data"]["State"].HasMember("Pid")) {
        logger.error("Pid field does not exist: " + json_str(res));
    }

    return res["data"]["State"]["Pid"].GetInt();
}

bool DockerAPI::is_cntr_running(const string &cntr_name) {
    auto res = this->inspect_cntr(cntr_name);
    if (!res["success"].GetBool()) {
        logger.error("inspect_cntr: " + json_str(res));
    }

    // Note that a running container can be paused. The Running and Paused
    // booleans are not mutually exclusive
    return res["data"]["State"]["Running"].GetBool();
}

pair<int, string> DockerAPI::exec(const string &cntr_name,
                                  const unordered_map<string, string> &envs,
                                  const vector<string> &cmd,
                                  const string &working_dir) {
    auto res = this->create_exec(cntr_name, envs, cmd, working_dir);
    if (!res["success"].GetBool()) {
        logger.error("create_exec: " + json_str(res));
    }

    string exec_id = res["data"]["Id"].GetString();

    res = this->start_exec(exec_id);
    if (!res["success"].GetBool()) {
        logger.error("start_exec: " + json_str(res));
    }

    res = this->inspect_exec(exec_id);
    if (!res["success"].GetBool()) {
        logger.error("inspect_exec: " + json_str(res));
    }

    return {res["data"]["Pid"].GetInt(), exec_id};
}

bool DockerAPI::is_exec_running(const string &exec_id) {
    auto res = this->inspect_exec(exec_id);
    if (!res["success"].GetBool()) {
        logger.error("inspect_exec: " + json_str(res));
    }

    return res["data"]["Running"].GetBool();
}
