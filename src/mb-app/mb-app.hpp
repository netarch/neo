#pragma once

class MB_App
{
protected:
    friend class Config;
    MB_App() = default;

public:
    virtual ~MB_App() = default;

    /* Note that all internal states should be flushed/reset */
    virtual void init() = 0;    // hard-reset, restart, start
    virtual void reset() = 0;   // soft-reset, reload
};

void mb_app_init(MB_App *);
void mb_app_reset(MB_App *);
