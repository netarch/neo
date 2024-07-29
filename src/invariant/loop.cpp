#include "invariant/loop.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <tuple>

#include "eqclass.hpp"
#include "invariant/invariant.hpp"
#include "lib/hash.hpp"
#include "logger.hpp"
#include "model-access.hpp"
#include "node.hpp"
#include "process/forwarding.hpp"
#include "protocols.hpp"

using namespace std;

string Loop::to_string() const {
    return "Loop invariant";
}

void Loop::init() {
    Invariant::init();
    model.set_violated(false);
}

static inline bool request_is_accepted(int mode, int proto_state) {
    return (mode == fwd_mode::ACCEPTED && PS_IS_REQUEST(proto_state)) ||
           (mode == fwd_mode::FIRST_FORWARD && PS_REQ_ACK_OR_REP(proto_state));
}

static inline bool packet_is_dropped(int mode) {
    return mode == fwd_mode::DROPPED;
}

static inline bool other_executable_conns_exist(const Model &model) {
    int current_conn = model.get_conn();
    for (int conn = 0; conn < model.get_num_conns(); ++conn) {
        if (conn != current_conn && model.get_executable_for_conn(conn) > 0) {
            return true;
        }
    }
    return false;
}

int Loop::check_violation() {
    int mode = model.get_fwd_mode();
    int proto_state = model.get_proto_state();
    EqClass *dst_ip_ec = model.get_dst_ip_ec();
    uint16_t dst_port = model.get_dst_port();
    Node *current_node = model.get_pkt_location();
    auto current_hop = make_tuple(dst_ip_ec, dst_port, current_node);
    const VisitedHops *const visited_hops = model.get_visited_hops();

    if ((request_is_accepted(mode, proto_state) || packet_is_dropped(mode)) &&
        !other_executable_conns_exist(model)) {
        // The current connection is accepted or dropped. If none of the other
        // connections are executable up to this point, no violation is
        // possible.
        model.set_violated(false);
        model.set_choice_count(0);
        return POL_NULL;
    }

    if (visited_hops == nullptr) {
        // A new beginning.
        VisitedHops new_hops;
        new_hops.add(std::move(current_hop));
        model.set_visited_hops(std::move(new_hops));
    } else if (visited_hops->visited(current_hop)) {
        // Found a forwarding loop. A violation occurred.
        model.set_violated(true);
        model.set_choice_count(0);
    } else if (visited_hops->is_last_hop(current_hop)) {
        // The current hop is the same as the last hop. The current packet is
        // most likely dropped or accepted, but some other connections are still
        // executable. Do nothing in this case.
    } else {
        // No loop has been found so far.
        // Create a new set of visited hops.
        VisitedHops new_hops(*visited_hops);
        new_hops.add(std::move(current_hop));
        model.set_visited_hops(std::move(new_hops));
    }

    return POL_NULL;
}

bool VisitedHops::visited(
    const std::tuple<EqClass *, uint16_t, Node *> &hop) const {
    return _hops.count(hop) > 0 && _last_hop != hop;
}

bool VisitedHops::is_last_hop(
    const std::tuple<EqClass *, uint16_t, Node *> &hop) const {
    return _last_hop == hop;
}

void VisitedHops::add(std::tuple<EqClass *, uint16_t, Node *> &&hop) {
    auto res = _hops.insert(std::move(hop));
    if (!res.second) {
        logger.error("Adding a duplicate hop. Loop invariant should have been "
                     "violated!");
    }
}

size_t VisitedHopsHash::operator()(const VisitedHops *const &hops) const {
    size_t value = 0;
    std::hash<EqClass *> ec_hasher;
    std::hash<uint16_t> port_hasher;
    std::hash<Node *> node_hasher;
    for (const auto &hop : hops->_hops) {
        ::hash::hash_combine(value, ec_hasher(std::get<0>(hop)));
        ::hash::hash_combine(value, port_hasher(std::get<1>(hop)));
        ::hash::hash_combine(value, node_hasher(std::get<2>(hop)));
    }
    ::hash::hash_combine(value, ec_hasher(std::get<0>(hops->_last_hop)));
    ::hash::hash_combine(value, port_hasher(std::get<1>(hops->_last_hop)));
    ::hash::hash_combine(value, node_hasher(std::get<2>(hops->_last_hop)));
    return value;
}

bool VisitedHopsEq::operator()(const VisitedHops *const &a,
                               const VisitedHops *const &b) const {
    return a->_hops == b->_hops && a->_last_hop == b->_last_hop;
}
