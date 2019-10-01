#pragma once

#include "mb-env/mb-env.hpp"
#include "node.hpp"

class NetNS : public MB_Env
{
private:
    int old_net, new_net;

public:
    NetNS(const Node *);
    ~NetNS() override;

    void run(void (*)(MB_App *), MB_App *) override;
};
