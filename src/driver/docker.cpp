#include "driver/docker.hpp"

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <list>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <sched.h>
#include <set>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <curl/curl.h>
#include <pcapplusplus/PcapFileDevice.h>

#include "dockerapi.hpp"
#include "dockernode.hpp"
#include "interface.hpp"
#include "lib/net.hpp"
#include "logger.hpp"
#include "middlebox.hpp"
#include "node.hpp"
#include "packet.hpp"
#include "pktbuffer.hpp"
#include "routingtable.hpp"

using namespace std;
namespace fs = boost::filesystem;

Docker::Docker(DockerNode *node, bool log_pkts)
    : _node(node), _dapi(node->daemon()), _pid(0), _log_pkts(log_pkts),
      _hnet_fd(-1), _cnet_fd(-1), _hmnt_fd(-1), _cmnt_fd(-1), _epollfd(-1),
      _events(nullptr) {}

Docker::~Docker() {
    this->teardown();
}

void Docker::set_interfaces() {
    struct ifreq ifr;
    int tapfd, ctrl_sock;

    // open a ctrl_sock for setting up IP addresses
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        logger.error("socket()", errno);
    }

    for (const auto &[addr, intf] : _node->get_intfs_l3()) {
        // create a new tap device
        if ((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
            logger.error("/dev/net/tun", errno);
        }
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        if (ioctl(tapfd, TUNSETIFF, &ifr) < 0) {
            close(tapfd);
            goto error;
        }
        this->_tapfds.emplace(intf, tapfd);

        // set up IP address
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, intf->get_name().c_str(), IFNAMSIZ - 1);
        ifr.ifr_addr.sa_family = AF_INET;
        ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr =
            htonl(intf->addr().get_value());
        if (ioctl(ctrl_sock, SIOCSIFADDR, &ifr) < 0) {
            goto error;
        }

        // set up network mask
        ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr =
            htonl(intf->mask().get_value());
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
        this->_macs.emplace(intf, mac);

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
    logger.error(ifr.ifr_name, errno);
}

void Docker::set_rttable() {
    const RoutingTable &rib = _node->get_rib();
    int ctrl_sock;
    struct rtentry rt;
    char intf_name[IFNAMSIZ];

    // open a ctrl_sock for setting up routing entries
    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        logger.error("socket()", errno);
    }

    for (const Route &route : rib) {
        memset(&rt, 0, sizeof(rt));

        // flags
        rt.rt_flags = RTF_UP;
        // network address
        ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr =
            htonl(route.get_network().network_addr().get_value());
        // network mask
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr =
            htonl(route.get_network().mask().get_value());
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
            ((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr =
                htonl(route.get_next_hop().get_value());
            rt.rt_flags |= RTF_GATEWAY;
        }

        if (ioctl(ctrl_sock, SIOCADDRT, &rt) < 0) {
            close(ctrl_sock);
            logger.error(route.to_string(), errno);
        }
    }

    close(ctrl_sock);
}

void Docker::set_arp_cache() {
    // Open a ctrl_sock for setting up arp cache entries
    int ctrl_sock;

    if ((ctrl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        logger.error("socket()", errno);
    }

    // Collect L3 peer IP addresses and egress interface
    set<pair<IPv4Address, Interface *>> peers;

    for (auto intf : _node->get_intfs()) {
        // find L3 peer
        auto l2peer = _node->get_peer(intf.first);

        if (l2peer.first) { // if the interface is truly connected
            if (!l2peer.second->is_l2()) {
                // L2 peer == L3 peer
                peers.emplace(l2peer.second->addr(), intf.second);
            } else {
                // pure L2 peer, find all L3 peers in the L2 LAN
                L2_LAN *l2_lan = l2peer.first->get_l2lan(l2peer.second);

                for (auto l3_endpoint : l2_lan->get_l3_endpoints()) {
                    const pair<Node *, Interface *> &l3peer =
                        l3_endpoint.second;
                    if (l3peer.second != intf.second) {
                        peers.emplace(l3peer.second->addr(), intf.second);
                    }
                }
            }
        }
    }

    // Set permanent arp cache entries
    struct arpreq arp;
    arp.arp_pa = {AF_INET, {0}};
    arp.arp_ha = {ARPHRD_ETHER, {0}};
    arp.arp_flags = ATF_COM | ATF_PERM;
    arp.arp_netmask = {AF_UNSPEC, {0}};
    uint8_t id_mac[6] = ID_ETH_ADDR;
    memcpy(arp.arp_ha.sa_data, id_mac, 6);

    for (const auto &[addr, intf] : peers) {
        ((struct sockaddr_in *)&arp.arp_pa)->sin_addr.s_addr =
            htonl(addr.get_value());
        strncpy(arp.arp_dev, intf->get_name().c_str(), 15);
        arp.arp_dev[15] = '\0';

        if (ioctl(ctrl_sock, SIOCSARP, &arp) < 0) {
            close(ctrl_sock);
            logger.error("Failed to set ARP cache for " + addr.to_string(),
                         errno);
        }
    }

    close(ctrl_sock);
}

void Docker::set_epoll_events() {
    // create an epoll instance for IO multiplexing
    if ((_epollfd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
        logger.error("epoll_create1", errno);
    }

    // register interesting interface fds in the epoll instance
    struct epoll_event event;

    for (const auto &[intf, tapfd] : this->_tapfds) {
        event.events = EPOLLIN;
        event.data.ptr = intf;
        if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, tapfd, &event) < 0) {
            logger.error("epoll_ctl", errno);
        }
    }

    // allocate space for receiving events
    _events = new struct epoll_event[this->_tapfds.size()];
}

void Docker::fetchns() {
    if (_pid <= 0) {
        logger.error("Invalid pid " + to_string(_pid));
    }

    // Save the host's namespace fds
    string nspath = "/proc/self/ns/net";
    if ((_hnet_fd = open(nspath.c_str(), O_RDONLY)) < 0) {
        logger.error(nspath, errno);
    }

    nspath = "/proc/self/ns/mnt";
    if ((_hmnt_fd = open(nspath.c_str(), O_RDONLY)) < 0) {
        logger.error(nspath, errno);
    }

    // Save the container's namespace fds
    nspath = "/proc/" + to_string(_pid) + "/ns/net";
    if ((_cnet_fd = open(nspath.c_str(), O_RDONLY)) < 0) {
        logger.error(nspath, errno);
    }

    nspath = "/proc/" + to_string(_pid) + "/ns/mnt";
    if ((_cmnt_fd = open(nspath.c_str(), O_RDONLY)) < 0) {
        logger.error(nspath, errno);
    }
}

void Docker::closens() {
    if (_hnet_fd >= 0) {
        close(_hnet_fd);
        _hnet_fd = -1;
    }

    if (_cnet_fd >= 0) {
        close(_cnet_fd);
        _cnet_fd = -1;
    }

    if (_hmnt_fd >= 0) {
        close(_hmnt_fd);
        _hmnt_fd = -1;
    }

    if (_cmnt_fd >= 0) {
        close(_cmnt_fd);
        _cmnt_fd = -1;
    }
}

void Docker::enterns(bool mnt) const {
    if (_hnet_fd < 0 || _cnet_fd < 0 || _hmnt_fd < 0 || _cmnt_fd < 0) {
        return;
    }

    if (setns(_cnet_fd, CLONE_NEWNET) < 0) {
        logger.error("setns()", errno);
    }

    if (mnt) {
        if (setns(_cmnt_fd, CLONE_NEWNS) < 0) {
            logger.error("setns()", errno);
        }
    }
}

void Docker::leavens(bool mnt) const {
    if (_hnet_fd < 0 || _cnet_fd < 0 || _hmnt_fd < 0 || _cmnt_fd < 0) {
        return;
    }

    if (setns(_hnet_fd, CLONE_NEWNET) < 0) {
        logger.error("setns()", errno);
    }

    if (mnt) {
        if (setns(_hmnt_fd, CLONE_NEWNS) < 0) {
            logger.error("setns()", errno);
        }
    }
}

void Docker::exec(const vector<string> &cmd) {
    if (this->_pid <= 0) {
        logger.error("Container isn't running");
    }

    auto res = this->_dapi.exec(_node->get_name(), _node->env_vars(), cmd,
                                _node->working_dir());
    this->_execs.emplace(std::move(res));
}

void Docker::teardown() {
    // Close pcap loggers
    for (auto &[_, pcap_logger] : this->_pcap_loggers) {
        pcap_logger->close();
    }
    this->_pcap_loggers.clear();

    enterns();

    // Epoll
    if (this->_epollfd >= 0) {
        close(this->_epollfd);
        this->_epollfd = -1;
    }

    if (this->_events) {
        delete[] this->_events;
        this->_events = nullptr;
    }

    // Delete the created tap devices
    for (const auto &[_, tapfd] : this->_tapfds) {
        close(tapfd);
    }
    this->_tapfds.clear();

    // Delete the allocated ethernet addresses
    for (const auto &[_, mac] : this->_macs) {
        delete[] mac;
    }
    this->_macs.clear();

    leavens();
    closens();

    // Terminate and remove container, it will also kill exec processes
    this->_dapi.remove_cntr(this->_node->get_name());
    this->_pid = 0;
    this->_execs.clear();
}

void Docker::init() {
    teardown();
    _pid = _dapi.run(*_node);
    fetchns();
    enterns();
    set_interfaces();   // Set tap interfaces and IP addresses
    set_rttable();      // Set routing table based on node.rib
    set_arp_cache();    // Set ARP entries
    set_epoll_events(); // Set epoll events for future packet reads
    leavens();

#ifdef ENABLE_DEBUG
    if (_log_pkts) {
        // Enable pcap logger for each interface
        for (const auto &[_, intf] : _node->get_intfs_l3()) {
            string pcapFn = to_string(getpid()) + "." + _node->get_name() +
                            "-" + intf->get_name() + ".pcap";
            this->_pcap_loggers.emplace(intf,
                                        make_unique<pcpp::PcapFileWriterDevice>(
                                            pcapFn, pcpp::LINKTYPE_ETHERNET));
            auto &pcapLogger = this->_pcap_loggers.at(intf);
            bool appendMode = fs::exists(pcapFn);
            if (!pcapLogger->open(appendMode)) {
                logger.error("Failed to open " + pcapFn);
            }
        }
    }
#endif
}

void Docker::reset() {
    // Reset teardown
    // Restarting a container changes the namespaces
    // So _tapfds, ns fds, _epollfd, _events also need to be reset
    {
        enterns();

        // Epoll
        if (this->_epollfd >= 0) {
            close(this->_epollfd);
            this->_epollfd = -1;
        }

        if (this->_events) {
            delete[] this->_events;
            this->_events = nullptr;
        }

        // Delete the created tap devices
        for (const auto &[_, tapfd] : this->_tapfds) {
            close(tapfd);
        }
        this->_tapfds.clear();

        // Delete the allocated ethernet addresses
        for (const auto &[_, mac] : this->_macs) {
            delete[] mac;
        }
        this->_macs.clear();

        leavens();
        closens();
    }

    // Restart the container, it will also kill exec processes
    _dapi.restart_cntr(_node->get_name());
    _pid = _dapi.get_cntr_pid(_node->get_name());
    _execs.clear();
    fetchns();
    enterns();
    set_interfaces();   // Set tap interfaces and IP addresses
    set_rttable();      // Set routing table based on node.rib
    set_arp_cache();    // Set ARP entries
    set_epoll_events(); // Set epoll events for future packet reads
    leavens();
}

size_t Docker::inject_packet(const Packet &pkt) {
    // Caution: libnet_init would fail within the container's mntns
    enterns();

    // Serialize the packet
    uint8_t *buf;
    uint32_t len;
    uint8_t src_mac[6] = ID_ETH_ADDR;
    uint8_t dst_mac[6] = {0};
    memcpy(dst_mac, this->_macs.at(pkt.get_intf()), sizeof(uint8_t) * 6);
    Net::get().serialize(&buf, &len, pkt, src_mac, dst_mac);

    // Write to the tap device fd
    int fd = this->_tapfds.at(pkt.get_intf());
    ssize_t nwrite = write(fd, buf, len);
    if (nwrite < 0) {
        logger.error("Packet injection failed", errno);
    }

    // Write to pcap loggers
    if (this->_pcap_loggers.count(pkt.get_intf()) > 0) {
        auto &pcapLogger = this->_pcap_loggers.at(pkt.get_intf());
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        pcpp::RawPacket rawpkt(buf, len, ts, false);
        pcapLogger->writePacket(rawpkt);
        pcapLogger->flush();
    }

    // Free resources
    Net::get().free(buf);

    leavens();
    return nwrite;
}

list<Packet> Docker::read_packets() const {
    list<PktBuffer> pktbuffs;

    enterns();

    // Wait until at least one of the fds becomes available
    int nfds = epoll_wait(_epollfd, _events, this->_tapfds.size(), -1);
    if (nfds < 0) {
        if (errno == EINTR) { // SIGUSR1 - stop thread
            return list<Packet>();
        }
        logger.error("epoll_wait", errno);
    }

    // Read from available tap fds
    for (int i = 0; i < nfds; ++i) {
        Interface *interface = static_cast<Interface *>(_events[i].data.ptr);
        int tapfd = this->_tapfds.at(interface);
        PktBuffer pktbuff(interface);
        ssize_t nread;
        if ((nread = read(tapfd, pktbuff.get_buffer(), pktbuff.get_len())) <
            0) {
            logger.error("Failed to read packet", errno);
        }
        pktbuff.set_len(nread);
        pktbuffs.push_back(pktbuff);
    }

    leavens();

    // Deserialize the packets
    list<Packet> pkts;
    for (const PktBuffer &pb : pktbuffs) {
        Packet pkt;
        Net::get().deserialize(pkt, pb);
        if (!pkt.empty()) {
            pkts.push_back(pkt);

            // Write to pcap loggers
            if (this->_pcap_loggers.count(pb.get_intf()) > 0) {
                auto &pcapLogger = this->_pcap_loggers.at(pb.get_intf());
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                pcpp::RawPacket rawpkt(pb.get_buffer(), pb.get_len(), ts,
                                       false);
                pcapLogger->writePacket(rawpkt);
                pcapLogger->flush();
            }
        }
    }

    return pkts;
}

/************* Code for creating a veth pair *************
 *********************************************************
    struct nl_sock *sock;
    struct rtnl_link *link, *peer;

    // create and connect to a netlink socket
    if (!(sock = nl_socket_alloc())) {
        logger.error("nl_socket_alloc", ENOMEM);
    }
    nl_connect(sock, NETLINK_ROUTE);

    for (auto pair : node->get_intfs_l3()) {
        Interface *intf = pair.second;

        // configure veth interfaces and put the peer to the original netns
        if (!(link = rtnl_link_veth_alloc())) {
            logger.error("rtnl_link_veth_alloc", ENOMEM);
        }
        peer = rtnl_link_veth_get_peer(link);
        rtnl_link_set_name(link, intf->get_name().c_str());
        rtnl_link_set_name(peer, (node->get_name() + "-"
                                  + intf->get_name()).c_str());
        rtnl_link_set_ns_fd(peer, old_net);

        // actually create the interfaces
        int err = rtnl_link_add(sock, link, NLM_F_CREATE);
        if (err < 0) {
            logger.error(string("rtnl_link_add: ") + nl_geterror(err));
        }

        // release the interface handles
        rtnl_link_put(peer);
        rtnl_link_put(link);
    }

    // release the socket connection
    nl_close(sock);
    nl_socket_free(sock);
**********************************************************/
