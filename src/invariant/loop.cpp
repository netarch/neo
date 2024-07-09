#include "invariant/loop.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <tuple>

#include "eqclass.hpp"
#include "invariant/invariant.hpp"
#include "lib/hash.hpp"
#include "model-access.hpp"
#include "node.hpp"

using namespace std;

string Loop::to_string() const {
    return "Loop invariant";
}

void Loop::init() {
    Invariant::init();
    model.set_violated(false);
}

int Loop::check_violation() {
    EqClass *dst_ip_ec = model.get_dst_ip_ec();
    uint16_t dst_port  = model.get_dst_port();
    Node *current_node = model.get_pkt_location();
    auto current_hop   = make_tuple(dst_ip_ec, dst_port, current_node);
    const VisitedHops *const visited_hops = model.get_visited_hops();

    if (visited_hops->visited(current_hop)) {
        // Found a forwarding loop. A violation occurred.
        model.set_violated(true);
        model.set_choice_count(0);
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
    return _hops.count(hop) > 0;
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
    return value;
}

bool VisitedHopsEq::operator()(const VisitedHops *const &a,
                               const VisitedHops *const &b) const {
    return a->_hops == b->_hops;
}
