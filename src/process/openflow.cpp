#include "process/openflow.hpp"

#include <cstring>
#include <cassert>

#include "lib/hash.hpp"
#include "model.h"

size_t OpenflowUpdateState::num_of_installed_updates(int node_order) const
{
    return this->update_vector.at(node_order);
}

void OpenflowUpdateState::install_update_at(int node_order)
{
    this->update_vector.at(node_order)++;
}

bool operator==(const OpenflowUpdateState& a, const OpenflowUpdateState& b)
{
    return a.update_vector == b.update_vector;
}

size_t OFUpdateStateHash::operator()(const OpenflowUpdateState *const& s) const
{
    return hash::hash(s->update_vector.data(),
                      s->update_vector.size() * sizeof(size_t));
}

bool OFUpdateStateEq::operator()(const OpenflowUpdateState *const& a,
                                 const OpenflowUpdateState *const& b) const
{
    return *a == *b;
}

void OpenflowProcess::add_update(Node *node, Route&& route)
{
    this->updates[node].push_back(route);
}

OpenflowProcess::~OpenflowProcess()
{
    for (auto& update_state : this->update_state_hist) {
        delete update_state;
    }
}

const decltype(OpenflowProcess::updates)& OpenflowProcess::get_updates() const
{
    return updates;
}

bool OpenflowProcess::has_updates(State *state, Node *node) const
{
    auto itr = this->updates.find(node);

    if (itr == this->updates.end()) {
        return false;
    }

    OpenflowUpdateState *update_state;
    memcpy(&update_state, state->comm_state[state->comm].openflow_update_state,
           sizeof(OpenflowUpdateState *));
    int node_order = std::distance(this->updates.begin(), itr);
    size_t num_installed = update_state->num_of_installed_updates(node_order);
    size_t num_of_all_updates = itr->second.size();

    if (num_installed < num_of_all_updates) {
        return true;
    }
    return false;   // all updates have been installed
}

void OpenflowProcess::init(State *state)
{
    if (this->updates.empty()) {
        this->disable();
        return;
    }

    OpenflowUpdateState *initial_state
        = new OpenflowUpdateState(this->updates.size());
    auto res = this->update_state_hist.insert(initial_state);
    assert(res.second == true);
    memcpy(state->comm_state[state->comm].openflow_update_state,
           &initial_state,
           sizeof(OpenflowUpdateState *));
}

void OpenflowProcess::reset(State *state)
{
    if (!enabled) {
        return;
    }

    /* reset openflow_update_state */
    OpenflowUpdateState *initial_state
        = new OpenflowUpdateState(this->updates.size());
    auto res = this->update_state_hist.insert(initial_state);
    if (!res.second) {
        delete initial_state;
        initial_state = *(res.first);
    }
    memcpy(state->comm_state[state->comm].openflow_update_state,
           &initial_state,
           sizeof(OpenflowUpdateState *));
}

void OpenflowProcess::exec_step(State *state, Network& network)
{
    if (!enabled) {
        return;
    }

    this->install_update(state, network);
}

/*
 * For now, setting choice_count to 1 means not continuing installing updates
 * since there is no update left, 2 otherwise.
 */
void OpenflowProcess::install_update(State *state, Network& network)
{
    if (state->choice == 0) {
        state->choice_count = 1; // back to forwarding
    }

    Node *current_node;
    memcpy(&current_node, state->comm_state[state->comm].pkt_location,
           sizeof(Node *));
    auto itr = this->updates.find(current_node);
    assert(itr != this->updates.end());
    const std::vector<Route>& all_updates = itr->second;
    int node_order = std::distance(this->updates.begin(), itr);

    OpenflowUpdateState *update_state;
    memcpy(&update_state,
           state->comm_state[state->comm].openflow_update_state,
           sizeof(OpenflowUpdateState *));

    size_t num_installed = update_state->num_of_installed_updates(node_order);

    // the openflow rule to be updated
    const Route& update = all_updates[num_installed];

    // actually install the update to FIB
    network.update_fib_openflow(state, current_node, update);

    // construct the new openflow update state after the install
    OpenflowUpdateState *new_update_state
        = new OpenflowUpdateState(*update_state);
    new_update_state->install_update_at(node_order);
    // insert into the pool of history update_state's
    auto res = this->update_state_hist.insert(new_update_state);
    if (!res.second) {
        delete new_update_state;
        new_update_state = *(res.first);
    }
    // update the openflow_update_state in Spin
    memcpy(state->comm_state[state->comm].openflow_update_state,
           &new_update_state,
           sizeof(OpenflowUpdateState *));

    if (num_installed + 1 < all_updates.size()) {
        // if there are still more updates of the current node,
        // non-deterministically install each of them
        state->choice_count = 2; // whether to install an update or not
    } else {
        state->choice_count = 1; // back to forwarding
    }
}

/*
OpenflowUpdateState *get_offset_vector(State *state)
{
    std::vector<size_t> *res;
    memcpy(&res, state->comm_state[state->comm].openflow_update_state,
           sizeof(OpenflowUpdateState *));
    return res;
}

// how many updates are yet to be installed on node
size_t get_update_size(State *state, const std::string& node) const;
{
    auto itr = updates.find(node);

    if (itr != updates.end()) {
        auto total_updates = itr->second.size();
        auto offset_vector_idx = std::distance(itr, updates.begin());
        const std::vector<size_t>& offset_vector = *(get_offset_vector(state));
        auto installed_updates = offset_vector[offset_vector_idx];
        assert(installed_updates <= total_updates);
        return total_updates - installed_updates;
    } else {
        return 0;
    }
}

bool has_updates(State *state, const std::string& node)
{
    return Openflow::get_update_size(node, state) > 0;
}
*/
