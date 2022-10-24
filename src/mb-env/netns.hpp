#pragma once

#include <list>
#include <string>
#include <sys/epoll.h>
#include <unordered_map>

#include "mb-env/mb-env.hpp"
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

    void set_env_vars(const std::string &node_name = "");
    void set_interfaces(const Node &);
    void set_rttable(const RoutingTable &);
    void set_arp_cache(const Node &);
    void set_epoll_events();

public:
    NetNS();
    ~NetNS() override;

    void init(const Node &) override;
    void run(void (*)(MB_App *), MB_App *) override;
    size_t inject_packet(const Packet &) override;
    std::list<Packet> read_packets() const override;
};
