#pragma once

#include <list>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#include <PcapFileDevice.h>

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
    DockerNode *_node;      // emulated node
    std::string _cntr_name; // name of the docker container
    DockerAPI _dapi;        // docker API object
    pid_t _pid;             // pid of the running process in the container
    bool _log_pkts;         // whether to log packets in debug mode
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
    void fetchns(); // Save namespace fds
    void closens(); // Close namespace fds

public:
    Docker(DockerNode *, bool log_pkts = true);
    ~Docker() override;

    decltype(_pid) pid() const { return _pid; }

    // A process can't join a new mount namespace if it is sharing
    // filesystem-related attributes (the attributes whose sharing is controlled
    // by the clone(2) CLONE_FS flag) with another process.
    // (https://man7.org/linux/man-pages/man2/setns.2.html)
    // ! Do not call with mnt = true when there are other threads !
    void enterns(bool mnt = false) const; // Enter the container namespaces
    void leavens(bool mnt = false) const; // Return to the original namespaces
    ino_t netns_ino() const;
    void exec(const std::vector<std::string> &cmd);
    void teardown(); // Reset the object

    void init() override;  // (Re)Initialize a docker container
    void reset() override; // (Soft-)Reset the container for backtracking
    void pause() override;
    void unpause() override;
    size_t inject_packet(const Packet &) override;
    std::list<Packet> read_packets() const override;
};
