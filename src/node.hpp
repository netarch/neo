#pragma once

#include <string>
#include <map>
#include <set>
#include <memory>

#include "interface.hpp"
#include "routingtable.hpp"
class Node;
#include "l2-lan.hpp"
#include "fib.hpp"

class Node
{
protected:
    std::string name;

    // interfaces indexed by name and ip address
    std::map<std::string, Interface *> intfs;       // all interfaces
    std::map<IPv4Address, Interface *> intfs_l3;    // L3 interfaces
    std::set<Interface *> intfs_l2;                 // L2 interfaces/switchport

    RoutingTable rib;   // RIB of this node read from the config

    // active connected L2 peers indexed by interface name
    std::map<std::string, std::pair<Node *, Interface *> > l2_peers;

    // L2 interfaces to L2 LANs mappings
    std::map<Interface *, L2_LAN *> l2_lans;

protected:
    friend class Config;
    Node() = default;
    void add_interface(Interface *interface);

public:
    Node(const Node&) = delete;
    Node(Node&&) = default;
    virtual ~Node();

    virtual Node& operator=(const Node&) = delete;
    virtual Node& operator=(Node&&) = default;

    virtual void init();    // per process initialization
    virtual std::string to_string() const;
    virtual std::string get_name() const;
    virtual bool has_ip(const IPv4Address& addr) const;
    virtual bool is_l3_only() const;

    virtual Interface *get_interface(const std::string&) const;
    virtual Interface *get_interface(const char *) const;
    virtual Interface *get_interface(const IPv4Address&) const;
    virtual const std::map<std::string, Interface *>& get_intfs() const;
    virtual const std::map<IPv4Address, Interface *>& get_intfs_l3() const;
    virtual const std::set<Interface *>& get_intfs_l2() const;
    virtual const RoutingTable& get_rib() const;
    virtual RoutingTable& get_rib();

    virtual std::pair<Node *, Interface *>
    get_peer(const std::string& intf_name) const;
    virtual void add_peer(const std::string&, Node *, Interface *);

    virtual bool mapped_to_l2lan(Interface *) const;
    virtual void set_l2lan(Interface *, L2_LAN *);
    virtual L2_LAN *get_l2lan(Interface *) const;

    /*
     * Compute the IP next hops from this node for a given destination address
     * by recursively looking up in the given RIB.
     */
    virtual std::set<FIB_IPNH> get_ipnhs(const IPv4Address&);

    /*
     * Compute the IP next hop from this node for a given egress interface and a
     * destination address.
     */
    virtual FIB_IPNH get_ipnh(const std::string& egress_intf_name,
                              const IPv4Address&);
};
