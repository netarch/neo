#pragma once

#include <chrono>

/*
 * Don't start emulation processes in the constructor.
 * Only read the configurations in constructors and later start the emulation in
 * init().
 */
class MB_App
{
protected:
    std::chrono::microseconds timeout;

public:
    virtual ~MB_App() = default;

    std::chrono::microseconds get_timeout() const;

    /* Note that all internal states should be flushed/reset */
    virtual void init() = 0;    // hard-reset, restart, start
    virtual void reset() = 0;   // soft-reset, reload
};

void mb_app_init(MB_App *);
void mb_app_reset(MB_App *);