#pragma once

#include <chrono>
#include <cstdint>

#include "driver/driver.hpp"
#include "packet.hpp"

class DropDetection {
protected:
    bool _enabled;

public:
    DropDetection() : _enabled(false) {}
    virtual ~DropDetection() = default;

    decltype(_enabled) enabled() const { return _enabled; }

    virtual void init() { _enabled = true; }
    virtual void teardown() { _enabled = false; }
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void start_listening_for(const Packet &pkt, Driver *driver) = 0;
    virtual uint64_t get_drop_ts(
        std::chrono::microseconds timeout = std::chrono::microseconds{-1}) = 0;
    virtual void stop_listening() = 0;
};

extern DropDetection *drop;
