#include "comm.hpp"

#include <cassert>

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

void Communication::compute_ecs(const EqClasses& all_ECs,
                                const EqClasses& owned_ECs)
{
    if (owned_dst_only) {
        ECs.add_mask_range(pkt_dst, owned_ECs);
    } else {
        ECs.add_mask_range(pkt_dst, all_ECs);
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
    return ECs.size();
}

size_t Communication::num_start_nodes() const
{
    return start_nodes.size();
}

bool Communication::set_initial_ec()
{
    if (!initial_ec || ++ECs_itr == ECs.end()) {
        ECs_itr = ECs.begin();
        initial_ec = *ECs_itr;
        assert(initial_ec != nullptr);
        return false;
    }
    initial_ec = *ECs_itr;
    assert(initial_ec != nullptr);
    return true;
}

EqClass *Communication::get_initial_ec() const
{
    return initial_ec;
}
