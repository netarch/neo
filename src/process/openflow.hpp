#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>

#include "process/process.hpp"
class Policy;
class Node;
class Route;
class Network;
class FIB_IPNH;
struct State;

/*
 * The state of how many updates have been installed.
 */
class OpenflowUpdateState
{
private:
    // index: node order in Openflow::updates
    // value: number of installed updates of that node
    std::vector<size_t> update_vector;

    friend struct OFUpdateStateHash;
    friend struct OFUpdateStateEq;

public:
    OpenflowUpdateState(size_t num_nodes): update_vector(num_nodes, 0) {}
    OpenflowUpdateState(const OpenflowUpdateState&) = default;
    OpenflowUpdateState(OpenflowUpdateState&&) = default;

    size_t num_of_installed_updates(int node_order) const;
    void install_update_at(int node_order);
};

struct OFUpdateStateHash {
    size_t operator()(const OpenflowUpdateState *const&) const;
};

struct OFUpdateStateEq {
    bool operator()(const OpenflowUpdateState *const&,
                    const OpenflowUpdateState *const&) const;
};

/*
 * An openflow process handles the openflow updates within one verification
 * session.
 */
class OpenflowProcess : public Process
{
private:
    std::map<Node *, std::vector<Route>> updates;

    void install_update(State *);

private:
    friend class Config;
    void add_update(Node *, Route&&);

public:
    OpenflowProcess() = default;

    std::string to_string() const;
    size_t num_updates() const; // number of total updates
    size_t num_nodes() const;   // number of nodes that have updates
    const decltype(updates)& get_updates() const;
    std::map<Node *, std::set<FIB_IPNH>> get_installed_updates(State *) const;
    bool has_updates(State *, Node *) const;

    void init(State *);
    void exec_step(State *, Network&) override;
};
