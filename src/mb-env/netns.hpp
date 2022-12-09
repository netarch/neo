#pragma once

#include <list>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unordered_map>

#include <pcapplusplus/PcapFileDevice.h>

#include "mb-env/mb-env.hpp"
#include "middlebox.hpp"

class Interface;
class Node;
class RoutingTable;
class Packet;

class NetNS : public MB_Env {
private:
    int old_net, new_net;
    int epollfd;
    struct epoll_event *events;
    std::string xtables_lockpath;
    std::unordered_map<Interface *, int> tapfds;        // intf --> tapfd
    std::unordered_map<Interface *, uint8_t *> tapmacs; // intf --> mac addr
    std::unordered_map<Interface *, std::unique_ptr<pcpp::PcapFileWriterDevice>>
        pcapLoggers;

    void set_env_vars(const std::string &node_name = "");
    void set_interfaces(const Node &);
    void set_rttable(const RoutingTable &);
    void set_arp_cache(const Node &);
    void set_epoll_events();

public:
    NetNS();
    ~NetNS() override;

    void init(const Middlebox &) override;
    void run(void (*)(MB_App *), MB_App *) override;
    size_t inject_packet(const Packet &) override;
    std::list<Packet> read_packets() const override;
};
