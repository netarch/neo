#include "node.hpp"

#include <utility>

#include "logger.hpp"

void Node::add_interface(Interface *interface) {
    // Add the new interface to intfs
    auto res =
        this->intfs.insert(std::make_pair(interface->get_name(), interface));
    if (res.second == false) {
        logger.error("Duplicate interface name: " + res.first->first);
    }

    if (interface->is_l2()) {
        // Add the new interface to intfs_l2
        this->intfs_l2.insert(interface);
    } else {
        // Add the new interface to intfs_l3
        auto res =
            this->intfs_l3.insert(std::make_pair(interface->addr(), interface));
        if (res.second == false) {
            logger.error("Duplicate interface IP: " +
                         res.first->first.to_string());
        }

        // Add the directly connected route to rib
        this->rib.emplace(interface->network(), interface->addr(),
                          interface->get_name(), 0);
    }
}

Node::~Node() {
    for (const auto &intf : intfs) {
        delete intf.second;
    }
}

std::string Node::to_string() const {
    return name;
}

std::string Node::get_name() const {
    return name;
}

bool Node::has_ip(const IPv4Address &addr) const {
    return intfs_l3.count(addr) > 0;
}

bool Node::is_l3_only() const {
    return intfs_l2.empty();
}

bool Node::is_emulated() const {
    return typeid(*this) != typeid(Node);
}

Interface *Node::get_interface(const std::string &intf_name) const {
    auto intf = intfs.find(intf_name);
    if (intf == intfs.end()) {
        logger.error(to_string() + " doesn't have interface " + intf_name);
    }
    return intf->second;
}

Interface *Node::get_interface(const char *intf_name) const {
    return get_interface(std::string(intf_name));
}

Interface *Node::get_interface(const IPv4Address &addr) const {
    auto intf = intfs_l3.find(addr);
    if (intf == intfs_l3.end()) {
        logger.error(to_string() + " doesn't own " + addr.to_string());
    }
    return intf->second;
}

Interface *Node::loopback_intf() const {
    auto itr1 = intfs.find("lo");
    if (itr1 != intfs.end()) {
        return itr1->second;
    }

    auto itr2 = intfs_l3.find("127.0.0.1");
    if (itr2 != intfs_l3.end()) {
        return itr2->second;
    }

    if (!intfs_l3.empty()) {
        return intfs_l3.begin()->second;
    }

    if (!intfs_l2.empty()) {
        return *intfs_l2.begin();
    }

    return nullptr;
}

const std::map<std::string, Interface *> &Node::get_intfs() const {
    return intfs;
}

const std::map<IPv4Address, Interface *> &Node::get_intfs_l3() const {
    return intfs_l3;
}

const std::set<Interface *> &Node::get_intfs_l2() const {
    return intfs_l2;
}

const RoutingTable &Node::get_rib() const {
    return rib;
}

RoutingTable &Node::get_rib() {
    return rib;
}

std::pair<Node *, Interface *>
Node::get_peer(const std::string &intf_name) const {
    auto peer = l2_peers.find(intf_name);
    if (peer == l2_peers.end()) {
        return std::make_pair(nullptr, nullptr);
    }
    return peer->second;
}

void Node::add_peer(const std::string &intf_name, Node *node, Interface *intf) {
    auto res =
        l2_peers.insert(std::make_pair(intf_name, std::make_pair(node, intf)));
    if (res.second == false) {
        logger.error("Two peers on interface: " + intf_name);
    }
}

bool Node::mapped_to_l2lan(Interface *intf) const {
    return l2_lans.count(intf) > 0;
}

void Node::set_l2lan(Interface *intf, L2_LAN *l2_lan) {
    l2_lans[intf] = l2_lan;
}

L2_LAN *Node::get_l2lan(Interface *intf) const {
    return l2_lans.at(intf);
}

/**
 * It looks up the next hop for a given destination address, and returns a set
 * of next hops
 *
 * @param dst the destination IP address
 * @param rib the RoutingTable to use for recursive lookups. If nullptr, use
 * this node's RoutingTable.
 * @param looked_up_ips a set of IP addresses that have already been looked up.
 *
 * @return A set of FIB_IPNHs
 */
std::set<FIB_IPNH>
Node::get_ipnhs(const IPv4Address &dst,
                const RoutingTable *rib,
                std::unordered_set<IPv4Address> *looked_up_ips) {
    std::set<FIB_IPNH> next_hops;
    std::unordered_set<IPv4Address> empty_set;

    if (looked_up_ips == nullptr) {
        looked_up_ips = &empty_set;
    } else if (looked_up_ips->count(dst) > 0) {
        return next_hops;
    }

    auto r = rib ? rib->lookup(dst) : this->rib.lookup(dst);
    looked_up_ips->insert(dst);

    for (RoutingTable::const_iterator it = r.first; it != r.second; ++it) {
        if (it->get_intf().empty()) {
            // non-connected route; look it up recursively
            auto nhs = this->get_ipnhs(it->get_next_hop(), rib, looked_up_ips);
            next_hops.insert(nhs.begin(), nhs.end());
        } else if (has_ip(dst)) {
            // connected route; accept
            next_hops.insert(FIB_IPNH(this, nullptr, this, nullptr));
            //                        l3nh l3nh_intf l2nh l2nh_intf
        } else {
            // connected route; forward
            FIB_IPNH next_hop = this->get_ipnh(it->get_intf(), dst);
            if (next_hop.l3_node()) { // next hop does exist
                next_hops.insert(next_hop);
            }
        }
    }

    return next_hops;
}

FIB_IPNH
Node::get_ipnh(const std::string &egress_intf_name, const IPv4Address &dst) {
    auto l2nh = get_peer(egress_intf_name); // L2 next hop
    if (l2nh.first) { // if the interface is truly connected (has peer)
        if (!l2nh.second->is_l2()) { // L2 next hop == L3 next hop
            return FIB_IPNH(l2nh.first, l2nh.second, l2nh.first, l2nh.second);
        } else {
            L2_LAN *l2_lan = l2nh.first->get_l2lan(l2nh.second);
            auto l3nh = l2_lan->find_l3_endpoint(dst);
            if (l3nh.first) {
                return FIB_IPNH(l3nh.first, l3nh.second, l2nh.first,
                                l2nh.second);
            }
        }
    }

    return FIB_IPNH(nullptr, nullptr, nullptr, nullptr);
}
