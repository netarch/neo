#include "mb-env/netns.hpp"

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/route.h>

#include "lib/logger.hpp"

void NetNS::set_interfaces(const Node *node)
{
    struct ifreq ifr;
    int tapfd, ctrl_sock;

    // open a ctrl_sock for setting up IP addresses
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        Logger::get().err("socket", errno);
    }

    for (const auto& pair : node->get_intfs_l3()) {
        Interface *intf = pair.second;

        // create a new tap device
        if ((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
            Logger::get().err("/dev/net/tun", errno);
        }
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP;
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        if (ioctl(tapfd, TUNSETIFF, &ifr) < 0) {
            close(tapfd);
            goto error;
        }
        //if (ioctl(fd, TUNSETPERSIST, 1) == -1) {
        //    Logger::get().warn(intf->get_name(), errno);
        //}
        tapfds.emplace(intf, tapfd);

        // set up IP address
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        ifr.ifr_addr.sa_family = AF_INET;
        ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr
            = htonl(intf->addr().get_value());
        if (ioctl(ctrl_sock, SIOCSIFADDR, &ifr) < 0) {
            goto error;
        }
        // set up network mask
        ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr
            = htonl(intf->mask().get_value());
        if (ioctl(ctrl_sock, SIOCSIFNETMASK, &ifr) < 0) {
            goto error;
        }
        // save ethernet address
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        ifr.ifr_addr.sa_family = AF_INET;
        if (ioctl(ctrl_sock, SIOCGIFHWADDR, &ifr) < 0) {
            goto error;
        }
        uint8_t *mac = new uint8_t[6];
        memcpy(mac, ifr.ifr_hwaddr.sa_data, sizeof(uint8_t) * 6);
        tapmacs.emplace(intf, mac);

        // bring up the interface
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_UP;
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        if (ioctl(ctrl_sock, SIOCSIFFLAGS, &ifr) < 0) {
            goto error;
        }
    }

    close(ctrl_sock);
    return;

error:
    close(ctrl_sock);
    Logger::get().err(ifr.ifr_name, errno);
}

void NetNS::set_rttable(const RoutingTable& rib)
{
    int ctrl_sock;
    struct rtentry rt;
    char intf_name[IFNAMSIZ];

    // open a ctrl_sock for setting up routing entries
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        Logger::get().err("socket", errno);
    }

    for (const Route& route : rib) {
        memset(&rt, 0, sizeof(rt));

        // flags
        rt.rt_flags = RTF_UP;
        // network address
        ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr
            = htonl(route.get_network().network_addr().get_value());
        // network mask
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr
            = htonl(route.get_network().mask().get_value());
        if (route.get_network().network_addr() ==
                route.get_network().broadcast_addr()) {
            rt.rt_flags |= RTF_HOST;
        }
        // gateway or rt_dev (next hop)
        if (!route.get_intf().empty()) {
            strncpy(intf_name, route.get_intf().c_str(), IFNAMSIZ - 1);
            rt.rt_dev = intf_name;
        } else {
            ((struct sockaddr_in *)&rt.rt_gateway)->sin_family = AF_INET;
            ((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr
                = htonl(route.get_next_hop().get_value());
            rt.rt_flags |= RTF_GATEWAY;
        }

        if (ioctl(ctrl_sock, SIOCADDRT, &rt) < 0) {
            close(ctrl_sock);
            Logger::get().err(route.to_string(), errno);
        }
    }

    close(ctrl_sock);
}

NetNS::NetNS(): old_net(-1), new_net(-1)
{
}

NetNS::~NetNS()
{
    if (old_net < 0 || new_net < 0) {
        return;
    }

    // delete the created tap devices
    for (const auto& tap : tapfds) {
        close(tap.second);
    }
    // delete the allocated ethernet addresses
    for (const auto& mac : tapmacs) {
        delete mac.second;
    }

    close(new_net);
}

void NetNS::init(const Node *node)
{
    const char *netns_path = "/proc/self/ns/net";

    // save the original netns fd
    if ((old_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get().err(netns_path, errno);
    }
    // create and enter a new netns
    if (unshare(CLONE_NEWNET) < 0) {
        Logger::get().err("Failed to create a new netns", errno);
    }
    // save the new netns fd
    if ((new_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get().err(netns_path, errno);
    }
    // create tap interfaces and set IP addresses
    set_interfaces(node);
    // update routing table according to node->rib
    set_rttable(node->get_rib());
    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get().err("Failed to setns", errno);
    }
}

void NetNS::run(void (*app_action)(MB_App *), MB_App *app)
{
    // enter the isolated netns
    if (setns(new_net, CLONE_NEWNET) < 0) {
        Logger::get().err("Failed to setns", errno);
    }

    app_action(app);

    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get().err("Failed to setns", errno);
    }
}

void NetNS::get_mac(Interface *intf, uint8_t *mac) const
{
    memcpy(mac, tapmacs.at(intf), sizeof(uint8_t) * 6);
}

size_t NetNS::inject_packet(Interface *intf, const uint8_t *buf, size_t len)
{
    int fd = tapfds.at(intf);
    ssize_t nwrite = write(fd, buf, len);
    if (nwrite < 0) {
        Logger::get().err("Packet injection failed", errno);
    }
    return nwrite;
}

/************* Code for creating a veth pair *************
 *********************************************************
    struct nl_sock *sock;
    struct rtnl_link *link, *peer;

    // create and connect to a netlink socket
    if (!(sock = nl_socket_alloc())) {
        Logger::get().err("nl_socket_alloc", ENOMEM);
    }
    nl_connect(sock, NETLINK_ROUTE);

    for (auto pair : node->get_intfs_l3()) {
        Interface *intf = pair.second;

        // configure veth interfaces and put the peer to the original netns
        if (!(link = rtnl_link_veth_alloc())) {
            Logger::get().err("rtnl_link_veth_alloc", ENOMEM);
        }
        peer = rtnl_link_veth_get_peer(link);
        rtnl_link_set_name(link, intf->get_name().c_str());
        rtnl_link_set_name(peer, (node->get_name() + "-"
                                  + intf->get_name()).c_str());
        rtnl_link_set_ns_fd(peer, old_net);

        // actually create the interfaces
        int err = rtnl_link_add(sock, link, NLM_F_CREATE);
        if (err < 0) {
            Logger::get().err(std::string("rtnl_link_add: ") +
                                       nl_geterror(err));
        }

        // release the interface handles
        rtnl_link_put(peer);
        rtnl_link_put(link);
    }

    // release the socket connection
    nl_close(sock);
    nl_socket_free(sock);
**********************************************************/
