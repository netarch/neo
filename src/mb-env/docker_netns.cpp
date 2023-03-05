#include "docker_netns.hpp"

#include <fcntl.h>

#include <boost/filesystem.hpp>

#include "logger.hpp"
#include "mb-app/docker.hpp"

using namespace std;
namespace fs = boost::filesystem;

void Docker_NetNS::init(const Middlebox &node) {
    const char *netns_path = "/proc/self/ns/net";

    // save the original netns fd
    if ((old_net = open(netns_path, O_RDONLY)) < 0) {
        logger.error(netns_path, errno);
    }
    // create and enter a new netns
    auto docker_node = dynamic_cast<Docker *>(node.get_app());

    std::string docker_netns_path = docker_node->net_ns_path();
    netns_path = docker_netns_path.c_str();

    // save the new netns fd
    if ((new_net = open(netns_path, O_RDONLY)) < 0) {
        logger.error(netns_path, errno);
    }
    set_env_vars(node.get_name()); // set environment variables
    set_interfaces(node);          // create tap interfaces and set IP addresses
    set_rttable(node.get_rib());   // set routing table according to node->rib
    set_arp_cache(node);           // set ARP entries
    set_epoll_events();            // set epoll events for future packet reads
    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        logger.error("setns()", errno);
    }

#ifdef ENABLE_DEBUG
    if (node.packet_logging_enabled()) {
        for (const auto &[_, intf] : node.get_intfs_l3()) {
            std::string pcapFn = std::to_string(getpid()) + "." +
                                 node.get_name() + "-" + intf->get_name() +
                                 ".pcap";
            pcapLoggers.emplace(intf, make_unique<pcpp::PcapFileWriterDevice>(
                                          pcapFn, pcpp::LINKTYPE_ETHERNET));
            auto &pcapLogger = pcapLoggers.at(intf);
            bool appendMode = fs::exists(pcapFn);
            if (!pcapLogger->open(appendMode)) {
                logger.error("Failed to open " + pcapFn);
            }
        }
    }
#endif
}

Docker_NetNS::~Docker_NetNS() {
    if (old_net < 0 || new_net < 0) {
        return;
    }

    // enter the isolated netns
    if (setns(new_net, CLONE_NEWNET) < 0) {
        logger.error("setns()", errno);
    }

    // epoll
    close(epollfd);
    delete[] events;
    // delete the created tap devices
    for (const auto &[intf, tapfd] : tapfds) {
        close(tapfd);
    }
    // delete the allocated ethernet addresses
    for (const auto &[intf, mac] : tapmacs) {
        delete[] mac;
    }
    // close pcap loggers
    for (auto &[intf, pcapLogger] : pcapLoggers) {
        pcapLogger->close();
    }

    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        logger.error("setns()", errno);
    }
    // close the new network namespace (docker will remove it)
    close(new_net);
}
