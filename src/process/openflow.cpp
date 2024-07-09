#include "process/openflow.hpp"

#include <cassert>

#include "fib.hpp"
#include "invariant/invariant.hpp"
#include "lib/hash.hpp"
#include "logger.hpp"
#include "model-access.hpp"
#include "node.hpp"

size_t OpenflowUpdateState::num_of_installed_updates(int node_order) const {
    return this->update_vector.at(node_order);
}

void OpenflowUpdateState::install_update_at(int node_order) {
    this->update_vector.at(node_order)++;
}

size_t
OFUpdateStateHash::operator()(const OpenflowUpdateState *const &s) const {
    return hash::hash(s->update_vector.data(),
                      s->update_vector.size() * sizeof(size_t));
}

bool OFUpdateStateEq::operator()(const OpenflowUpdateState *const &a,
                                 const OpenflowUpdateState *const &b) const {
    return a->update_vector == b->update_vector;
}

void OpenflowProcess::add_update(Node *node, Route &&route) {
    this->updates[node].push_back(route);
}

std::string OpenflowProcess::to_string() const {
    std::string ret = "=== Openflow updates:";

    for (const auto &update : this->updates) {
        ret += "\n--- " + update.first->get_name();
        for (const Route &route : update.second) {
            ret += "\n    " + route.to_string();
        }
    }

    return ret;
}

size_t OpenflowProcess::num_updates() const {
    size_t num = 0;
    for (const auto &pair : this->updates) {
        num += pair.second.size();
    }
    return num;
}

size_t OpenflowProcess::num_nodes() const {
    return this->updates.size();
}

const decltype(OpenflowProcess::updates) &OpenflowProcess::get_updates() const {
    return updates;
}

std::map<Node *, std::set<FIB_IPNH>>
OpenflowProcess::get_installed_updates() const {
    std::map<Node *, std::set<FIB_IPNH>> installed_updates;
    EqClass *ec                       = model.get_dst_ip_ec();
    OpenflowUpdateState *update_state = model.get_openflow_update_state();

    size_t node_order = 0;
    for (const auto &pair : this->updates) {
        size_t num_installed =
            update_state->num_of_installed_updates(node_order);
        Node *node          = pair.first;
        RoutingTable of_rib = node->get_rib();

        for (size_t i = 0; i < num_installed; ++i) {
            if (pair.second[i].relevant_to_ec(*ec)) {
                of_rib.update(pair.second[i]);
            }
        }

        IPv4Address addr             = ec->representative_addr();
        std::set<FIB_IPNH> next_hops = node->get_ipnhs(addr, &of_rib);
        if (!next_hops.empty()) {
            installed_updates.emplace(node, std::move(next_hops));
        }

        ++node_order;
    }

    return installed_updates;
}

bool OpenflowProcess::has_updates(Node *node) const {
    auto itr = this->updates.find(node);

    if (itr == this->updates.end()) {
        return false;
    }

    OpenflowUpdateState *update_state = model.get_openflow_update_state();
    int node_order       = std::distance(this->updates.begin(), itr);
    size_t num_installed = update_state->num_of_installed_updates(node_order);
    size_t num_of_all_updates = itr->second.size();

    if (num_installed < num_of_all_updates) {
        return true;
    }
    return false; // all updates have been installed
}

void OpenflowProcess::init() {
    if (this->updates.empty()) { // do nothing if there's no update
        return;
    }

    model.set_openflow_update_state(OpenflowUpdateState(this->updates.size()));
}

void OpenflowProcess::exec_step() {
    if (this->updates.empty()) { // do nothing if there's no update
        return;
    }

    this->install_update();
}

void OpenflowProcess::reset() {
    this->updates.clear();
}

/**
 * For now, setting choice_count to 1 means not continuing installing updates
 * since there is no update left, 2 otherwise.
 */
void OpenflowProcess::install_update() {
    if (model.get_choice() == 0) {
        logger.info("Openflow: not installing update");
        model.set_choice_count(1); // back to forwarding
        return;
    }

    Node *current_node = model.get_pkt_location();
    auto itr           = this->updates.find(current_node);
    assert(itr != this->updates.end());
    const std::vector<Route> &all_updates = itr->second;
    int node_order = std::distance(this->updates.begin(), itr);
    OpenflowUpdateState *update_state = model.get_openflow_update_state();
    size_t num_installed = update_state->num_of_installed_updates(node_order);

    // set the new openflow update state
    OpenflowUpdateState new_update_state(*update_state);
    new_update_state.install_update_at(node_order);
    model.set_openflow_update_state(std::move(new_update_state));

    // set choice_count
    if (num_installed + 1 < all_updates.size()) {
        // if there are still more updates of the current node,
        // non-deterministically install each of them
        model.set_choice_count(2); // whether to install an update or not
    } else {
        model.set_choice_count(1); // back to forwarding
    }

    // the openflow rule to be updated
    const Route &update = all_updates[num_installed];

    // actually install the update to FIB
    logger.info("Openflow: installing update at " + current_node->get_name() +
                ": " + update.to_string());

    // check route precedence (longest prefix match)
    RoutingTable of_rib = current_node->get_rib();
    EqClass *ec         = model.get_dst_ip_ec();
    for (size_t i = 0; i < num_installed; ++i) {
        if (all_updates[i].relevant_to_ec(*ec)) {
            of_rib.insert(all_updates[i]);
        }
    }
    of_rib.update(update);

    // get the next hops
    IPv4Address addr             = ec->representative_addr();
    std::set<FIB_IPNH> next_hops = current_node->get_ipnhs(addr, &of_rib);

    // construct the new FIB
    FIB fib(*model.get_fib());
    fib.set_ipnhs(current_node, std::move(next_hops));

    // update FIB
    FIB *new_fib = model.set_fib(std::move(fib));
    logger.debug(new_fib->to_string());
}
