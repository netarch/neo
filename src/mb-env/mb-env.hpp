#pragma once

#include "node.hpp"
#include "mb-app/mb-app.hpp"

class MB_Env
{
public:
    virtual ~MB_Env() = default;

    virtual void init(const Node *) = 0;
    virtual void run(void (*)(MB_App *), MB_App *) = 0;
};
