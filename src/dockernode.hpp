#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "middlebox.hpp"
#include "protocols.hpp"

struct DockerVolumeMount {
    std::string mount_path;
    std::string host_path;
    std::string driver;
    bool read_only;
};

class DockerNode : public Middlebox {
private:
    std::string _daemon;
    std::string _image;
    std::string _working_dir;
    std::vector<std::string> _cmd;
    std::vector<std::pair<proto, int>> _ports;
    std::unordered_map<std::string, std::string> _env_vars;
    std::vector<DockerVolumeMount> _mounts;
    std::unordered_map<std::string, std::string> _sysctls;

private:
    friend class ConfigParser;
    DockerNode() = default;

public:
    const decltype(_daemon) &daemon() const { return _daemon; }
    const decltype(_image) &image() const { return _image; }
    const decltype(_working_dir) &working_dir() const { return _working_dir; }
    const decltype(_cmd) &cmd() const { return _cmd; }
    const decltype(_ports) &ports() const { return _ports; }
    const decltype(_env_vars) &env_vars() const { return _env_vars; }
    const decltype(_mounts) &mounts() const { return _mounts; }
    const decltype(_sysctls) &sysctls() const { return _sysctls; }
};
