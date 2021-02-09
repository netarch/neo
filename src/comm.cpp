#include "comm.hpp"

#include <cassert>
#include <random>

Communication::Communication()
    : protocol(0), owned_dst_only(false), initial_ec(nullptr),
      src_port(0), dst_port(0)
{
}

int Communication::get_protocol() const
{
    return protocol;
}

const std::vector<Node *>& Communication::get_start_nodes() const
{
    return start_nodes;
}

std::string Communication::start_nodes_str() const
{
    std::string ret = "[";
    for (Node *node : start_nodes) {
        ret += " " + node->to_string();
    }
    ret += " ]";
    return ret;
}

uint16_t Communication::get_src_port() const
{
    return src_port;
}

uint16_t Communication::get_dst_port() const
{
    return dst_port;
}

void Communication::set_src_port(uint16_t src_port)
{
    this->src_port = src_port;
}

void Communication::set_dst_port(uint16_t dst_port)
{
    this->dst_port = dst_port;
}

void Communication::compute_ecs(const EqClasses& all_ECs,
                                const EqClasses& owned_ECs,
                                const std::set<uint16_t>& dst_ports)
{
    if (owned_dst_only) {
        ECs.add_mask_range(pkt_dst, owned_ECs);
    } else {
        ECs.add_mask_range(pkt_dst, all_ECs);
    }

    if (this->dst_ports.empty() && (protocol == proto::tcp || protocol == proto::udp)) {
        for (auto& port : dst_ports) {
            this->dst_ports.push_back(port);
        }
        uint16_t port;
        std::default_random_engine generator;
        std::uniform_int_distribution<uint16_t> distribution(10,49151);
        do {
            port = distribution(generator);
        } while (dst_ports.count(port) > 0);
        this->dst_ports.push_back(port);
    }
}

void Communication::add_ec(const IPv4Address& addr)
{
    ECs.add_ec(addr);
}

EqClass *Communication::find_ec(const IPv4Address& addr) const
{
    return ECs.find_ec(addr);
}

size_t Communication::num_ecs() const
{
    return ECs.size() * (dst_ports.size() ? dst_ports.size() : 1);
}

size_t Communication::num_start_nodes() const
{
    return start_nodes.size();
}

bool Communication::set_initial_ec()
{
    if (initial_ec && dst_ports_itr != dst_ports.end()) {
        ++dst_ports_itr;
        if (dst_ports_itr != dst_ports.end()) {
            dst_port = *dst_ports_itr;
            return true;
        }
        // dst_ports_itr == dst_ports.end()
    }

    if (!initial_ec || ++ECs_itr == ECs.end()) {
        ECs_itr = ECs.begin();
        initial_ec = *ECs_itr;
        assert(initial_ec != nullptr);
        dst_ports_itr = dst_ports.begin();
        if (dst_ports_itr != dst_ports.end()) {
            dst_port = *dst_ports_itr;
        }
        return false;
    }
    initial_ec = *ECs_itr;
    assert(initial_ec != nullptr);
    dst_ports_itr = dst_ports.begin();
    if (dst_ports_itr != dst_ports.end()) {
        dst_port = *dst_ports_itr;
    }
    return true;
}

EqClass *Communication::get_initial_ec() const
{
    return initial_ec;
}
