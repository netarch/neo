#pragma once

#include <unordered_map>
#include <vector>

#include "mb-env/mb-env.hpp"
#include "interface.hpp"
#include "node.hpp"
#include "routingtable.hpp"

class NetNS : public MB_Env
{
private:
    int old_net, new_net;
    std::unordered_map<Interface *, int> tapfds;        // intf --> tapfd
    std::unordered_map<Interface *, uint8_t *> tapmacs; // intf --> mac addr
    //const char *xtables_lock_mnt = "/run/xtables.lock";
    //char xtables_lock[25];  // "/tmp/xtables.lock.XXXXXX"

    void set_interfaces(const Node&);
    void set_rttable(const RoutingTable&);
    void set_arp_cache(const Node&);
    //void mntns_xtables_lock();

public:
    NetNS();
    ~NetNS() override;

    void init(const Node&) override;
    void run(void (*)(MB_App *), MB_App *) override;
    size_t inject_packet(const Packet&) override;
    std::vector<Packet> read_packets() const override;
};
