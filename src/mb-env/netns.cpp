#include "mb-env/netns.hpp"

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "lib/logger.hpp"

void NetNS::set_interfaces(const Node *node)
{
    struct ifreq ifr;
    int tapfd, ctrl_sock;

    // open a ctrl_sock for setting up IP addresses
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        Logger::get_instance().err("socket", errno);
    }

    for (const auto& pair : node->get_intfs_l3()) {
        Interface *intf = pair.second;

        // create a new tap device
        if ((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
            Logger::get_instance().err("/dev/net/tun", errno);
        }
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP;
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        if (ioctl(tapfd, TUNSETIFF, &ifr) < 0) {
            close(tapfd);
            goto error;
        }
        //if (ioctl(fd, TUNSETPERSIST, 1) == -1) {
        //    Logger::get_instance().warn(intf->get_name(), errno);
        //}
        tapfds.emplace(intf->get_name(), tapfd);

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
    }

    close(ctrl_sock);
    return;

error:
    close(ctrl_sock);
    Logger::get_instance().err(ifr.ifr_name, errno);
}

void NetNS::set_rttable(const RoutingTable& rib __attribute__((unused)))
{
    /*
    int ctrl_sock;
    struct rtentry rt;

    // open a ctrl_sock for setting up routing entries
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        Logger::get_instance().err("socket", errno);
    }

    for (const Route& route : rib) {
        memset(&rt, 0, sizeof(rt));

        // network
        rt.rt_dst

        // gateway
        rt.rt_gateway
    }

    close(ctrl_sock);
    */
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

    close(new_net);
}

void NetNS::init(const Node *node)
{
    const char *netns_path = "/proc/self/ns/net";

    // save the original netns fd
    if ((old_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get_instance().err(netns_path, errno);
    }
    // create and enter a new netns
    if (unshare(CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to create a new netns", errno);
    }
    // save the new netns fd
    if ((new_net = open(netns_path, O_RDONLY)) < 0) {
        Logger::get_instance().err(netns_path, errno);
    }
    // create tap interfaces and set IP addresses
    set_interfaces(node);
    // update routing table according to node->rib
    set_rttable(node->get_rib());
    system("ip a");
    system("ip r");
    exit(0);
    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }
}

void NetNS::run(void (*app_action)(MB_App *), MB_App *app)
{
    // enter the isolated netns
    if (setns(new_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }

    app_action(app);

    // return to the original netns
    if (setns(old_net, CLONE_NEWNET) < 0) {
        Logger::get_instance().err("Failed to setns", errno);
    }
}

/************* Code for creating a veth pair *************
 *********************************************************
    struct nl_sock *sock;
    struct rtnl_link *link, *peer;

    // create and connect to a netlink socket
    if (!(sock = nl_socket_alloc())) {
        Logger::get_instance().err("nl_socket_alloc", ENOMEM);
    }
    nl_connect(sock, NETLINK_ROUTE);

    for (auto pair : node->get_intfs_l3()) {
        Interface *intf = pair.second;

        // configure veth interfaces and put the peer to the original netns
        if (!(link = rtnl_link_veth_alloc())) {
            Logger::get_instance().err("rtnl_link_veth_alloc", ENOMEM);
        }
        peer = rtnl_link_veth_get_peer(link);
        rtnl_link_set_name(link, intf->get_name().c_str());
        rtnl_link_set_name(peer, (node->get_name() + "-"
                                  + intf->get_name()).c_str());
        rtnl_link_set_ns_fd(peer, old_net);

        // actually create the interfaces
        int err = rtnl_link_add(sock, link, NLM_F_CREATE);
        if (err < 0) {
            Logger::get_instance().err(std::string("rtnl_link_add: ") +
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
