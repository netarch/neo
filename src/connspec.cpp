#include "connspec.hpp"

#include "eqclassmgr.hpp"
#include "protocols.hpp"

ConnSpec::ConnSpec() : protocol(0), src_port(0), owned_dst_only(false) {}

void ConnSpec::update_policy_ecs() const {
    EqClassMgr::get().add_ec(dst_ip);
}

std::set<Connection> ConnSpec::compute_connections() const {
    std::set<Connection> conns;

    // compute dst IP ECs
    std::set<EqClass *> dst_ip_ecs =
        EqClassMgr::get().get_overlapped_ecs(dst_ip, owned_dst_only);

    // compute dst ports
    std::set<uint16_t> dst_ports;
    if (this->dst_ports.empty()) {
        if (protocol == proto::tcp || protocol == proto::udp) {
            dst_ports = EqClassMgr::get().get_ports();
        } else { // ICMP
            dst_ports.insert(0);
        }
    } else {
        dst_ports = this->dst_ports;
    }

    for (Node *src_node : this->src_nodes) {
        for (EqClass *dst_ip_ec : dst_ip_ecs) {
            for (uint16_t dst_port : dst_ports) {
                Connection conn(this->protocol, src_node, dst_ip_ec,
                                this->src_port, dst_port);
                conns.insert(std::move(conn));
            }
        }
    }

    return conns;
}
