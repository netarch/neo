#pragma once

#include <unordered_map>

#include "mb-env/mb-env.hpp"
#include "node.hpp"

class NetNS : public MB_Env
{
private:
    int old_net, new_net;
    std::unordered_map<Interface *, int> tapfds;    // intf --> tapfd
    std::unordered_map<Interface *, uint8_t *> tapmacs; // intf --> mac addr

    void set_interfaces(const Node *);
    void set_rttable(const RoutingTable&);

public:
    NetNS();
    ~NetNS() override;

    void init(const Node *) override;
    void run(void (*)(MB_App *), MB_App *) override;
    void get_mac(Interface *, uint8_t *) const override;
    size_t inject_packet(Interface *, const uint8_t *buf, size_t len) override;
    // TODO: read_packet
};
