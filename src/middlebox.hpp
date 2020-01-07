#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "mb-env/mb-env.hpp"
#include "mb-app/mb-app.hpp"
#include "pkt-hist.hpp"

class Middlebox : public Node
{
private:
    MB_Env *env;    // environment
    MB_App *app;    // appliance

    NodePacketHistory *node_pkt_hist;
    std::thread *listener;
    std::atomic<bool> listener_end;
    std::set<FIB_IPNH> next_hops;
    std::mutex mtx;                 // lock for accessing next_hops
    std::condition_variable cv;     // for reading next_hops

    void listen_packets();

public:
    Middlebox(const std::shared_ptr<cpptoml::table>&);
    Middlebox() = delete;
    Middlebox(const Middlebox&) = delete;
    Middlebox(Middlebox&&) = default;
    ~Middlebox() override;

    Middlebox& operator=(const Middlebox&) = delete;
    Middlebox& operator=(Middlebox&&) = default;

    /*
     * Actually initialize and start the emulation.
     */
    void init() override;

    void rewind(NodePacketHistory *);
    void set_node_pkt_hist(NodePacketHistory *);
    std::set<FIB_IPNH> send_pkt(const Packet&); // send one L7 packet

    /*
     * Return an empty set. We don't model the forwarding behavior of
     * middleboxes. The forwarding process will inject a concrete packet to the
     * emulation instance.
     */
    std::set<FIB_IPNH> get_ipnhs(const IPv4Address&) override;
};
