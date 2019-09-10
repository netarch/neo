#include "middlebox.hpp"
#include "mb-env/netns.hpp"
#include "mb-app/netfilter.hpp"

Middlebox::Middlebox(const std::shared_ptr<cpptoml::table>& node_config)
    : Node(node_config)
{
    auto environment = node_config->get_as<std::string>("env");
    auto appliance = node_config->get_as<std::string>("app");

    if (!environment) {
        Logger::get_instance().err("Missing environment");
    }
    if (!appliance) {
        Logger::get_instance().err("Missing appliance");
    }

    if (*environment == "netns") {
        env = new NetNS(this);
    } else {
        Logger::get_instance().err("Unknown environment: " + *environment);
    }

    if (*appliance == "netfilter") {
        app = new NetFilter(node_config);
    } else {
        Logger::get_instance().err("Unknown appliance: " + *appliance);
    }

    env->run(mb_app_init, app);
}

Middlebox::~Middlebox()
{
    delete app;
    delete env;
}

std::set<FIB_IPNH> Middlebox::get_ipnhs(
    const IPv4Address& dst __attribute__((unused)))
{
    return std::set<FIB_IPNH>();
}

std::set<FIB_IPNH> Middlebox::send_pkt(State *state __attribute__((unused)), const IPv4Address& dst_ip __attribute__((unused))) const
{
    // TODO
    // check state's pkt_hist
    // rewind and update state if needed
    // update pkt_hist with this injecting packet
    // inject packet
    // return next hop(s)
    return std::set<FIB_IPNH>();
}
