#pragma once

#include <cpptoml/cpptoml.hpp>

#include "node.hpp"
#include "mb-env/mb-env.hpp"
#include "mb-app/mb-app.hpp"
#include "pan.h"

class Middlebox : public Node
{
private:
    MB_Env *env;    // environment
    MB_App *app;    // appliance
    // pkt_hist

    //void rewind(NPH); // env->run(mb_app_reset, app) and replay history

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

    /*
     * Return an empty set. We don't model the forwarding behavior of
     * middleboxes. The forwarding process will inject a concrete packet to the
     * emulation instance.
     */
    std::set<FIB_IPNH> get_ipnhs(const IPv4Address&) override;

    std::set<FIB_IPNH> send_pkt(State *, const IPv4Address& dst_ip) const;
};
