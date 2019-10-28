#pragma once

#include <unordered_map>

#include "mb-env/mb-env.hpp"
#include "node.hpp"

class NetNS : public MB_Env
{
private:
    int old_net, new_net;
    std::unordered_map<std::string, int> tapfds;    // intf name --> tapfd

    void set_interfaces(const Node *);
    void set_rttable(const RoutingTable&);

public:
    NetNS();
    ~NetNS() override;

    void init(const Node *) override;
    void run(void (*)(MB_App *), MB_App *) override;
};
