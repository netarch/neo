#pragma once

#include <list>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

#include <pcapplusplus/PcapFileDevice.h>

#include "dockerapi.hpp"
#include "driver/driver.hpp"
#include "packet.hpp"

class DockerNode;
class Interface;
class Middlebox;
class Node;
class RoutingTable;

class Docker : public Driver {
private:
    DockerNode *_node; // emulated node
    DockerAPI _dapi;   // docker API object
    pid_t _pid;        // pid of the running process in the container
    bool _log_pkts;    // whether to log packets in debug mode
    std::unordered_map<pid_t, std::string> _execs;    // pid -> exec_id
    std::unordered_map<Interface *, int> _tapfds;     // intf --> tapfd
    std::unordered_map<Interface *, uint8_t *> _macs; // intf --> mac addr
    std::unordered_map<Interface *, std::unique_ptr<pcpp::PcapFileWriterDevice>>
        _pcap_loggers;

    // Namespace fds
    int _hnet_fd, _cnet_fd; // host/container netns fd
    int _hmnt_fd, _cmnt_fd; // host/container mntns fd

    // Epoll variables
    int _epollfd;
    struct epoll_event *_events;

    void set_interfaces();
    void set_rttable();
    void set_arp_cache();
    void set_epoll_events();

public:
    Docker(DockerNode *, bool log_pkts = true);
    ~Docker() override;

    decltype(_pid) pid() const { return _pid; }

    void fetchns();       // Save namespace fds
    void enterns() const; // Enter the container namespaces
    void leavens() const; // Return to the main process namespaces
    void closens();       // Close namespace fds
    void teardown();
    void exec(const std::vector<std::string> &cmd);

    // (Re)Initialize a docker container
    void init() override;
    // (Soft-)Reset the running container for state backtracking.
    void reset() override;

    size_t inject_packet(const Packet &) override;
    std::list<Packet> read_packets() const override;
};
